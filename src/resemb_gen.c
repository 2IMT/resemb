#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#define STREQ(l, r) (strcmp(l, r) == 0)
#define WRITE_OUT(...)                               \
        do {                                         \
          if (!write_out(__VA_ARGS__)) return false; \
        } while (0)
#define ALIGNMENT 8
#define ALIGN_UP(x) (((x) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

static const char* program_name = NULL;
static const char* output = NULL;
static const char* strip_prefix = NULL;
static size_t prefix_len = 0;
static const char** files = NULL;
static size_t file_count = 0;

static FILE* output_file = NULL;

static void help(void) {
  printf("USAGE:\n");
  printf("    %s [OPTIONS] <INPUT_FILES...>\n", program_name);
  printf("    %s -o resemb.c file_a.txt file_b.txt\n", program_name);
  printf("\n");
  printf("OPTIONS:\n");
  printf("    -o, --output <PATH>\n");
  printf("        Output to file.\n");
  printf("\n");
  printf("    -s, --strip-prefix <PREFIX>\n");
  printf("        Strip specified prefix from all resource names.\n");
  printf("        Filenames which don't contain specified prefix will be unchanged.\n");
  printf("\n");
  printf("        Example:\n");
  printf("        `%s -s foo/ foo/file_a.txt foo/file_b.txt file_c.txt`\n", program_name);
  printf("        The resulting package will contain this resources:\n");
  printf("        `file_a.txt` with contents of `foo/file_a.txt`\n");
  printf("        `file_b.txt` with contents of `foo/file_b.txt`\n");
  printf("        `file_c.txt` with contents of `file_c.txt`\n");
}

static void hint(void) {
  fprintf(stderr, "Use `%s --help` for the usage.\n", program_name);
}

static const char* strip_filename(const char* name) {
  if (!strip_prefix) return name;
  if (strncmp(name, strip_prefix, prefix_len) == 0)
    return name + prefix_len;
  return name;
}

static int name_comp(const void* l, const void* r) {
  const char* l_str = *(const char* const*)l;
  const char* r_str = *(const char* const*)r;
  if (strip_prefix) {
    l_str = strip_filename(l_str);
    r_str = strip_filename(r_str);
  }
  return strcmp(l_str, r_str);
}

static bool write_out(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int res = vfprintf(output_file, fmt, ap);
  va_end(ap);
  if (res < 0) {
    fprintf(stderr, "resemb_gen: error: failed to write to file `%s`: `%s`",
            output, strerror(errno));
    return false;
  }
  return true;
}

struct offset {
  unsigned int offset;
  unsigned int len;
};

static bool open_file(FILE** file, const char* filename, const char* modes) {
  FILE* opened = fopen(filename, modes);
  if (file == NULL) {
    fprintf(stderr, "resemb_gen: error: failed to open file `%s`: `%s`\n",
            filename, strerror(errno));
    return false;
  }
  *file = opened;
  return true;
}

static bool file_size(const char* filename, size_t* size) {
  FILE* file = fopen(filename, "r");
  if (!open_file(&file, filename, "rb")) return false;
  fseek(file, 0, SEEK_END);
  long pos = ftell(file);
  if (pos < 0) {
    fprintf(stderr, "resemb_gen: error: failed to read file `%s`: `%s`\n",
            filename, strerror(errno));
    return false;
  }
  fclose(file);
  *size = pos;
  return true;
}

static bool generate_names(void) {
  for (size_t i = 0; i < file_count; i++) {
    WRITE_OUT("  \"%s\",\n", strip_filename(files[i]));
  }
  return true;
}

static bool generate_offsets(struct offset* offsets) {
  for (size_t i = 0; i < file_count; i++) {
    WRITE_OUT("  { .offset = %u, .len = %u },\n",
              offsets[i].offset, offsets[i].len);
  }
  return true;
}

static bool generate_data(void) {
  size_t total_size = 0;
  for (size_t i = 0; i < file_count; i++) {
    FILE* file = NULL;
    if (!open_file(&file, files[i], "rb")) return false;
    size_t file_bytes_read = 0;
    WRITE_OUT("  ");
    while (!feof(file)) {
      char buffer[1024 * 8] = { 0 };
      unsigned long bytes_read = fread(buffer, 1, 1024 * 8, file);
      if (ferror(file)) {
        fprintf(
          stderr,
          "resemb_gen: error: failed to read file `%s`: `%s`\n",
          files[i], strerror(errno));
        fclose(file);
        return false;
      }
      for (size_t j = 0; j < bytes_read; j++) {
        WRITE_OUT("%d, ", (unsigned char) buffer[j]);
      }
      file_bytes_read += bytes_read;
    }
    fclose(file);
    total_size += file_bytes_read;
    total_size += 1; // for null-terminator
    WRITE_OUT("0, "); // null-terminator
    size_t total_size_aligned = ALIGN_UP(total_size);
    size_t padding = total_size_aligned - total_size;
    for (size_t j = 0; j < padding; j++) {
      WRITE_OUT("0, ");
    }
    WRITE_OUT("\n");
    total_size = total_size_aligned;
  }
  return true;
}

static bool generate(void) {
  struct offset* offsets = calloc(file_count, sizeof(struct offset));
  size_t offset_curr = 0;
  for (size_t i = 0; i < file_count; i++) {
    size_t size;
    if (!file_size(files[i], &size)) return false;
    size += 1; // for null-terminator
    offsets[i].offset = offset_curr;
    offsets[i].len = size - 1; // exclude null-terminator
    offset_curr += size;
    offset_curr = ALIGN_UP(offset_curr);
  }
  WRITE_OUT("#define RESEMB__RESOURCE_COUNT %zu\n", file_count);
  WRITE_OUT("#define RESEMB__DATA_SIZE %zu\n", offset_curr);
  WRITE_OUT("static const char* const resemb__names[RESEMB__RESOURCE_COUNT] = {\n");
  if (!generate_names()) return false;
  WRITE_OUT("};\n");
  WRITE_OUT("static const struct {\n");
  WRITE_OUT(" unsigned int offset;\n");
  WRITE_OUT(" unsigned int len;\n");
  WRITE_OUT("} resemb__offsets[RESEMB__RESOURCE_COUNT] = {\n");
  if (!generate_offsets(offsets)) return false;
  WRITE_OUT("};\n");
  WRITE_OUT("static const unsigned char resemb__data[RESEMB__DATA_SIZE] = {\n");
  if (!generate_data()) return false;
  WRITE_OUT("};\n");
  WRITE_OUT("static int resemb__strcmp(const char* l, const char* r) {\n");
  WRITE_OUT("  if (l == r) return 0;\n");
  WRITE_OUT("  for (unsigned int c = 0; ; c++) {\n");
  WRITE_OUT("    if (l[c] != r[c]) return (unsigned char) l[c] - (unsigned char) r[c];\n");
  WRITE_OUT("    if (l[c] == 0) return 0;\n");
  WRITE_OUT("  }\n");
  WRITE_OUT("  return 0;\n");
  WRITE_OUT("}\n");
  WRITE_OUT("static int resemb__search_binary(unsigned int* id, const char* name) {\n");
  WRITE_OUT("  unsigned int starti = 0;\n");
  WRITE_OUT("  unsigned int endi = (RESEMB__RESOURCE_COUNT > 0) ?\n");
  WRITE_OUT("                      (RESEMB__RESOURCE_COUNT - 1) : 0;\n");
  WRITE_OUT("  while (starti <= endi) {\n");
  WRITE_OUT("    unsigned int midi = starti + (endi - starti) / 2;\n");
  WRITE_OUT("    int cmp = resemb__strcmp(name, resemb__names[midi]);\n");
  WRITE_OUT("    if (cmp == 0) {\n");
  WRITE_OUT("      *id = midi;\n");
  WRITE_OUT("      return 1;\n");
  WRITE_OUT("    } else if (cmp < 0) {\n");
  WRITE_OUT("        if (midi == 0) break;\n");
  WRITE_OUT("      endi = midi - 1;\n");
  WRITE_OUT("    } else {\n");
  WRITE_OUT("      starti = midi + 1;\n");
  WRITE_OUT("    }\n");
  WRITE_OUT("  }\n");
  WRITE_OUT("  return 0;\n");
  WRITE_OUT("}\n");
  WRITE_OUT("typedef struct ResembRes {\n");
  WRITE_OUT("  unsigned int id;\n");
  WRITE_OUT("  const char* name;\n");
  WRITE_OUT("  const unsigned char* data;\n");
  WRITE_OUT("  unsigned int len;\n");
  WRITE_OUT("} ResembRes;\n");
  WRITE_OUT("ResembRes resemb_get_unchecked(unsigned int id) {\n");
  WRITE_OUT("  return (ResembRes) {\n");
  WRITE_OUT("    .id = id,\n");
  WRITE_OUT("    .name = resemb__names[id],\n");
  WRITE_OUT("    .data = resemb__data + resemb__offsets[id].offset,\n");
  WRITE_OUT("    .len = resemb__offsets[id].len,\n");
  WRITE_OUT("  };\n");
  WRITE_OUT("}\n");
  WRITE_OUT("int resemb_find(ResembRes *res, const char *name) {\n");
  WRITE_OUT("  if (RESEMB__RESOURCE_COUNT == 0) return 0;\n");
  WRITE_OUT("  unsigned int id;\n");
  WRITE_OUT("  if (resemb__search_binary(&id, name)) {\n");
  WRITE_OUT("    *res = resemb_get_unchecked(id);\n");
  WRITE_OUT("    return 1;\n");
  WRITE_OUT("  }\n");
  WRITE_OUT("  return 0;\n");
  WRITE_OUT("}\n");
  WRITE_OUT("int resemb_get(ResembRes* res, unsigned int id) {\n");
  WRITE_OUT("  if (id >= RESEMB__RESOURCE_COUNT) return 0;\n");
  WRITE_OUT("  *res = resemb_get_unchecked(id);\n");
  WRITE_OUT("  return 1;\n");
  WRITE_OUT("}\n");
  WRITE_OUT("unsigned int resemb_count(void) {\n");
  WRITE_OUT("  return RESEMB__RESOURCE_COUNT;\n");
  WRITE_OUT("}\n");
  WRITE_OUT("const char* const* resemb_list(void) {\n");
  WRITE_OUT("  return resemb__names;\n");
  WRITE_OUT("}\n");
  WRITE_OUT("unsigned int resemb_size(void) {\n");
  WRITE_OUT("  return RESEMB__DATA_SIZE;\n");
  WRITE_OUT("}\n");

  free(offsets);

  return true;
}

static void parse_args(int argc, char** argv) {
  program_name = argv[0];

  for (size_t i = 1; argv[i] != NULL; i++) {
    if (argv[i][0] == '-') {
      const char* arg = argv[i];
      if (STREQ(arg, "-h") || STREQ(arg, "--help")) {
        help();
        exit(0);
      } else if (STREQ(arg, "-o") || STREQ(arg, "--output")) {
        i++;
        if (argv[i] == NULL) {
          fprintf(stderr, "resemb_gen: error: `%s` requires an argument\n", arg);
          hint();
          exit(1);
        }
        output = argv[i];
      } else if (STREQ(arg, "-s") || STREQ(arg, "--strip-prefix")) {
        i++;
        if (argv[i] == NULL) {
          fprintf(stderr, "resemb_gen: error: `%s` requires an argument\n", arg);
          hint();
          exit(1);
        }
        strip_prefix = argv[i];
        prefix_len = strlen(strip_prefix);
      } else {
        fprintf(stderr, "resemb_gen: error: invalid argument `%s`\n", arg);
      }
    } else {
      files = (const char**) argv + i;
      file_count = argc - i;
      break;
    }
  }
  if (file_count == 0) {
    fprintf(stderr, "resemb_gen: error: input files required\n");
    hint();
    exit(1);
  }
}

int main(int argc, char** argv) {
  parse_args(argc, argv);
  if (output == NULL) {
    output_file = stdout;
  } else {
    if (!open_file(&output_file, output, "w")) return 1;
  }

  qsort(files, file_count, sizeof(const char*), name_comp);

  for (size_t i = 0; i < file_count; i++) {
    fprintf(stderr, "resemb_gen: storing `%s` as `%s`\n", files[i], strip_filename(files[i]));
  }

  if (!generate()) return 1;

  fclose(output_file);

  return 0;
}
