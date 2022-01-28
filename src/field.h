/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FIELD_H
#define SECP256K1_FIELD_H

#include "util.h"

/* This file defines the generic interface for working with secp256k1_fe
 * objects, which represent field elements (integers modulo 2^256 - 2^32 - 977).
 *
 * The actual definition of the secp256k1_fe type depends on the chosen field
 * implementation; see the field_5x52.h and field_10x26.h files for details.
 *
 * All secp256k1_fe objects have implicit properties that determine what
 * operations are permitted on it. These are purely a function of what
 * secp256k1_fe_ operations are applied on it, generally (implicitly) fixed at
 * compile time, and do not depend on the chosen field implementation. Despite
 * that, what these properties actually entail for the field representation
 * values depends on the chosen field implementation. These properties are:
 * - magnitude: an integer in [0,32]
 * - normalized: 0 or 1; normalized=1 implies magnitude <= 1.
 *
 * In VERIFY mode, they are materialized explicitly as fields in the struct,
 * allowing run-time verification of these properties. In that case, the field
 * implementation also provides a secp256k1_fe_verify routine to verify that
 * these fields match the run-time value and perform internal consistency
 * checks. */
#ifdef VERIFY
#  define SECP256K1_FE_VERIFY_FIELDS \
    int magnitude; \
    int normalized;
#else
#  define SECP256K1_FE_VERIFY_FIELDS
#endif

#if defined(SECP256K1_WIDEMUL_INT128)
#include "field_5x52.h"
#elif defined(SECP256K1_WIDEMUL_INT64)
#include "field_10x26.h"
#else
#error "Please select wide multiplication implementation"
#endif

#ifdef VERIFY
/* Magnitude and normalized value for constants. */
#define SECP256K1_FE_VERIFY_CONST(d7, d6, d5, d4, d3, d2, d1, d0) \
    /* Magnitude is 0 for constant 0; 1 otherwise. */ \
    , (((d7) | (d6) | (d5) | (d4) | (d3) | (d2) | (d1) | (d0)) != 0) \
    /* Normalized is 1 unless sum(d_i<<(32*i) for i=0..7) exceeds field modulus. */ \
    , (!(((d7) & (d6) & (d5) & (d4) & (d3) & (d2)) == 0xfffffffful && ((d1) == 0xfffffffful || ((d1) == 0xfffffffe && (d0 >= 0xfffffc2f)))))
#else
#define SECP256K1_FE_VERIFY_CONST(d7, d6, d5, d4, d3, d2, d1, d0)
#endif

/** This expands to an initializer for a secp256k1_fe valued sum((i*32) * d_i, i=0..7) mod p.
 *
 * It has magnitude 1, unless d_i are all 0, in which case the magnitude is 0.
 * It is normalized, unless sum(2^(i*32) * d_i, i=0..7) >= p.
 *
 * SECP256K1_FE_CONST_INNER is provided by the implementation.
 */
#define SECP256K1_FE_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {SECP256K1_FE_CONST_INNER((d7), (d6), (d5), (d4), (d3), (d2), (d1), (d0)) SECP256K1_FE_VERIFY_CONST((d7), (d6), (d5), (d4), (d3), (d2), (d1), (d0)) }

static const secp256k1_fe secp256k1_fe_one = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1);
static const secp256k1_fe secp256k1_const_beta = SECP256K1_FE_CONST(
    0x7ae96a2bul, 0x657c0710ul, 0x6e64479eul, 0xac3434e9ul,
    0x9cf04975ul, 0x12f58995ul, 0xc1396c28ul, 0x719501eeul
);

#ifndef VERIFY
/* In non-VERIFY mode, we #define the fe operations to be identical to their
 * internal field implementation, to avoid the potential overhead of a
 * function call (even though presumably inlinable). */
#  define secp256k1_fe_normalize secp256k1_fe_impl_normalize
#  define secp256k1_fe_normalize_weak secp256k1_fe_impl_normalize_weak
#  define secp256k1_fe_normalize_var secp256k1_fe_impl_normalize_var
#  define secp256k1_fe_normalizes_to_zero secp256k1_fe_impl_normalizes_to_zero
#  define secp256k1_fe_normalizes_to_zero_var secp256k1_fe_impl_normalizes_to_zero_var
#  define secp256k1_fe_set_int secp256k1_fe_impl_set_int
#  define secp256k1_fe_clear secp256k1_fe_impl_clear
#  define secp256k1_fe_is_zero secp256k1_fe_impl_is_zero
#  define secp256k1_fe_is_odd secp256k1_fe_impl_is_odd
#  define secp256k1_fe_cmp_var secp256k1_fe_impl_cmp_var
#  define secp256k1_fe_set_b32 secp256k1_fe_impl_set_b32
#  define secp256k1_fe_get_b32 secp256k1_fe_impl_get_b32
#  define secp256k1_fe_negate secp256k1_fe_impl_negate
#  define secp256k1_fe_mul_int secp256k1_fe_impl_mul_int
#endif /* !defined(VERIFY) */

/** Normalize a field element.
 *
 * On input, r must be a valid field element.
 * On output, r represents the same value but has normalized=1 and magnitude=1.
 */
static void secp256k1_fe_normalize(secp256k1_fe *r);

/** Give a field element magnitude 1.
 *
 * On input, r must be a valid field element.
 * On output, r represents the same value but has magnitude=1. Normalized is unchanged.
 */
static void secp256k1_fe_normalize_weak(secp256k1_fe *r);

/** Normalize a field element, without constant-time guarantee.
 *
 * Identical in behavior to secp256k1_fe_normalize, but not constant time in r.
 */
static void secp256k1_fe_normalize_var(secp256k1_fe *r);

/** Determine whether r represents field element 0.
 *
 * On input, r must be a valid field element.
 * Returns whether r = 0 (mod p).
 */
static int secp256k1_fe_normalizes_to_zero(const secp256k1_fe *r);

/** Determine whether r represents field element 0, without constant-time guarantee.
 *
 * Identical in behavior to secp256k1_normalizes_to_zero, but not constant time in r.
 */
static int secp256k1_fe_normalizes_to_zero_var(const secp256k1_fe *r);

/** Set a field element to an integer in range [0,0x7FFF].
 *
 * On input, r does not need to be initialized, a must be in [0,0x7FFF].
 * On output, r represents value a, is normalized and has magnitude (a!=0).
 */
static void secp256k1_fe_set_int(secp256k1_fe *r, int a);

/** Set a field element to 0.
 *
 * On input, a does not need to be initialized.
 * On output, a represents 0, is normalized and has magnitude 0.
 */
static void secp256k1_fe_clear(secp256k1_fe *a);

/** Determine whether a represents field element 0.
 *
 * On input, a must be a valid normalized field element.
 * Returns whether a = 0 (mod p).
 *
 * This behaves identical to secp256k1_normalizes_to_zero{,_var}, but requires
 * normalized input (and is much faster).
 */
static int secp256k1_fe_is_zero(const secp256k1_fe *a);

/** Determine whether a (mod p) is odd.
 *
 * On input, a must be a valid normalized field element.
 * Returns (int(a) mod p) & 1.
 */
static int secp256k1_fe_is_odd(const secp256k1_fe *a);

/** Determine whether two field elements are equal.
 *
 * On input, a and b must be valid field elements with magnitudes not exceeding
 * 1 and 31, respectively.
 * Returns a = b (mod p).
 */
static int secp256k1_fe_equal(const secp256k1_fe *a, const secp256k1_fe *b);

/** Determine whether two field elements are equal, without constant-time guarantee.
 *
 * Identical in behavior to secp256k1_fe_equal, but not constant time in either a or b.
 */
static int secp256k1_fe_equal_var(const secp256k1_fe *a, const secp256k1_fe *b);

/** Compare the values represented by 2 field elements, without constant-time guarantee.
 *
 * On input, a and b must be valid normalized field elements.
 * Returns 1 if a > b, -1 if a < b, and 0 if a = b (comparisons are done as integers
 * in range 0..p-1).
 */
static int secp256k1_fe_cmp_var(const secp256k1_fe *a, const secp256k1_fe *b);

/** Set a field element equal to a provided 32-byte big endian value.
 *
 * On input, r does not need to be initalized. a must be a pointer to an initialized 32-byte array.
 * On output, r = a (mod p). It will have magnitude 1, and if (a < p), it will be normalized.
 * If not, it will only be weakly normalized. Returns whether (a < p).
 *
 * Note that this function is unusual in that the normalization of the output depends on the
 * run-time value of a.
 */
static int secp256k1_fe_set_b32(secp256k1_fe *r, const unsigned char *a);

/** Convert a field element to 32-byte big endian byte array.
 * On input, a must be a valid normalized field element, and r a pointer to a 32-byte array.
 * On output, r = a (mod p).
 */
static void secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe *a);

/** Negate a field element.
 *
 * On input, r does not need to be initialized. a must be a valid field element with
 * magnitude not exceeding m. m must be an integer in [0,31].
 * Performs {r = -a}.
 * On output, r will not be normalized, and will have magnitude m+1.
 */
static void secp256k1_fe_negate(secp256k1_fe *r, const secp256k1_fe *a, int m);

/** Adds a small integer (up to 0x7FFF) to r. The resulting magnitude increases by one. */
static void secp256k1_fe_add_int(secp256k1_fe *r, int a);

/** Multiply a field element with a small integer.
 *
 * On input, r must be a valid field element. a must be an integer in [0,32].
 * The magnitude of r times a must not exceed 32.
 * Performs {r *= a}.
 * On output, r's magnitude is multiplied by a, and r will not be normalized.
 */
static void secp256k1_fe_mul_int(secp256k1_fe *r, int a);

/** Adds a field element to another. The result has the sum of the inputs' magnitudes as magnitude. */
static void secp256k1_fe_add(secp256k1_fe *r, const secp256k1_fe *a);

/** Sets a field element to be the product of two others. Requires the inputs' magnitudes to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_mul(secp256k1_fe *r, const secp256k1_fe *a, const secp256k1_fe * SECP256K1_RESTRICT b);

/** Sets a field element to be the square of another. Requires the input's magnitude to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_sqr(secp256k1_fe *r, const secp256k1_fe *a);

/** If a has a square root, it is computed in r and 1 is returned. If a does not
 *  have a square root, the root of its negation is computed and 0 is returned.
 *  The input's magnitude can be at most 8. The output magnitude is 1 (but not
 *  guaranteed to be normalized). The result in r will always be a square
 *  itself. */
static int secp256k1_fe_sqrt(secp256k1_fe *r, const secp256k1_fe *a);

/** Sets a field element to be the (modular) inverse of another. Requires the input's magnitude to be
 *  at most 8. The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_inv(secp256k1_fe *r, const secp256k1_fe *a);

/** Potentially faster version of secp256k1_fe_inv, without constant-time guarantee. */
static void secp256k1_fe_inv_var(secp256k1_fe *r, const secp256k1_fe *a);

/** Convert a field element to the storage type. */
static void secp256k1_fe_to_storage(secp256k1_fe_storage *r, const secp256k1_fe *a);

/** Convert a field element back from the storage type. */
static void secp256k1_fe_from_storage(secp256k1_fe *r, const secp256k1_fe_storage *a);

/** If flag is true, set *r equal to *a; otherwise leave it. Constant-time.  Both *r and *a must be initialized.*/
static void secp256k1_fe_storage_cmov(secp256k1_fe_storage *r, const secp256k1_fe_storage *a, int flag);

/** If flag is true, set *r equal to *a; otherwise leave it. Constant-time.  Both *r and *a must be initialized.*/
static void secp256k1_fe_cmov(secp256k1_fe *r, const secp256k1_fe *a, int flag);

/** Halves the value of a field element modulo the field prime. Constant-time.
 *  For an input magnitude 'm', the output magnitude is set to 'floor(m/2) + 1'.
 *  The output is not guaranteed to be normalized, regardless of the input. */
static void secp256k1_fe_half(secp256k1_fe *r);

/** Sets each limb of 'r' to its upper bound at magnitude 'm'. The output will also have its
 *  magnitude set to 'm' and is normalized if (and only if) 'm' is zero. */
static void secp256k1_fe_get_bounds(secp256k1_fe *r, int m);

/** Determine whether a is a square (modulo p). */
static int secp256k1_fe_is_square_var(const secp256k1_fe *a);

/** Check invariants on a field element (no-op unless VERIFY is enabled). */
static void secp256k1_fe_verify(const secp256k1_fe *a);

#endif /* SECP256K1_FIELD_H */
