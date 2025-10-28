/**
 * `resemb` resource finder.
 *
 * This module allows accessing resources embedded with `resemb`.
 * Note that you should link the generated `resemb` source before using
 * functions provided by this header.
 */

#ifndef RESEMB_RESEMB_H_
#define RESEMB_RESEMB_H_

/**
 * `resemb` resource.
 */
typedef struct ResembRes {
  /**
   * Resource ID.
   */
  unsigned int id;

  /**
   * Name of the resource.
   */
  const char* name;

  /**
   * Pointer to resource's data. Always null-terminated.
   */
  const unsigned char* data;

  /**
   * Length of the resource's data in bytes excluding null-terminator.
   */
  unsigned int len;
} ResembRes;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Find `resemb` resource.
 *
 * Params:
 * `res` - `resemb` resource to put data into.
 * `name` - Resource name.
 *
 * Returns `1` if found, `0` - otherwise.
 */
int resemb_find(ResembRes* res, const char* name);

/**
 * Get `resemb` resource by ID.
 *
 * Params:
 * `res` - `resemb` resource to put data into.
 * `id` - `resemb` resource ID.
 *
 * Returns `1` if ID is valid, `0` - otherwise.
 */
int resemb_get(ResembRes* res, unsigned int id);

/**
 * Get `resemb` resource by ID.
 * Acts like `resemb_get` but doesn't perform bound checks.
 * Note that out-of-bounds access results in undefined behavior.
 *
 * Params:
 * `id` - `resemb` resoure ID.
 *
 * Returns `resemb` resource.
 */
ResembRes resemb_get_unchecked(unsigned int id);

/**
 * Get the array of available resource names.
 * Index into the resulting array represents a `resemb` resource ID.
 */
const char* const* resemb_list(void);

/**
 * Get the total number of resources stored.
 */
unsigned int resemb_count(void);

/**
 * Get the total amount of bytes occupied by all resources.
 */
unsigned int resemb_size(void);

#ifdef __cplusplus
}
#endif

#endif // RESEMB_RESEMB_H_
