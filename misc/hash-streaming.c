/* This code is a thought experiment based on a talk Adam Langley (Google) gave
 * at Trust Unconference 2015. Adam proposed a variation on Merkle trees that
 * leaves you with some good options even in resource constrained environments
 * where you can't afford to buffer the entire data you're receiving to disk.
 *
 * Where a traditional Merkle tree looks like the following:
 *
 *        A        <-- root hash
 *       / \
 *      /   \
 *     B     C     <-- intermediate hashes
 *    / \   / \
 *   1  2  3   4   <-- data
 *
 * Adam proposed something like the following:
 *
 *   A             <-- root hash
 *   |\
 *   | B           <-- intermediate hashes
 *   | |\
 *   | | C         <-- intermediate hashes
 *   | | |\
 *   D E F G       <-- intermediate hashes
 *   | | | |
 *   1 2 3 4       <-- data
 *
 * In both schemes, the sender and the receiver both know A. In the second, the
 * sender transmits (1, B, 2, C, 3, 4). This allows the receiver to
 * incrementally validate what they're receiving, storing only a small amount
 * of data in memory.
 *
 * The following code attempts to implement this second scheme in my own
 * exercise to better understand the proposal.
 *
 * Disclaimer: I do not know Adam and the following code was written purely
 * based on my interpretation of his talk. Any mistakes are likely my own, not
 * his. This code is in the public domain. Feel free to use it in any way you
 * wish.
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Basic type for byte buffer with accompanying size. */
typedef struct {
    unsigned char *data;
    size_t length;
} buffer_t;

/* A hash function supplied by the caller. */
typedef buffer_t (*hash_fn)(buffer_t chunk);

/* State for authentication. */
typedef struct {

    /* Ratcheting counter. Index begins at zero and we do tick-tock of (1) flip
     * the hash member and (2) increment the index. See hash_tick below.
     */
    unsigned index;
    bool hash;

    /* The caller-supplied hashing function. */
    hash_fn hasher;

    /* The current target hash we have. This begins as the uppermost root of
     * the tree and is gradually lowered as we receive chunks.
     */
    buffer_t target_hash;

    /* The hash of the last block (if we are on a hash = false cycle) or
     * invalid otherwise/
     */
    buffer_t pending_hash;

} state_t;

/* Initialise a new state for streamining to begin. */
void hash_init(state_t *st, hash_fn hasher, buffer_t target_hash) {
    memset(st, 0, sizeof *st);
    st->hasher = hasher;
    st->target_hash.data = target_hash.data;
    st->target_hash.length = target_hash.length;
}

/* Ratchet the progress counter. */
static void hash_tick(state_t *st) {
    if (st->hash) {
        st->hash = false;
        st->index++;
    } else {
        st->hash = true;
    }
}

/* Take a step based on the block we just received. */
int hash_step(state_t *st, buffer_t block) {

    if (st->hash) {
        /* The current block we have is intended to be a hash. */

        assert(st->pending_hash.data != NULL);

        /* Concatenate the hash we have onto the pending hash and then hash the
         * result.
         */
        unsigned char amalgamated[st->pending_hash.length + block.length];
        memcpy(amalgamated, st->pending_hash.data, st->pending_hash.length);
        memcpy((void*)amalgamated + st->pending_hash.length, block.data, block.length);
        buffer_t buffer = {
            .data = amalgamated,
            .length = sizeof amalgamated,
        };
        buffer_t hash = st->hasher(buffer);

        /* If the hash did not match our current target, we have failed
         * authentication.
         */
        if (st->target_hash.length != hash.length ||
                memcmp(st->target_hash.data, hash.data, st->target_hash.length) != 0)
            return -1;

        /* Now that we validated the received block, stash it as the next target
         * hash.
         */
        st->target_hash = block;
        st->pending_hash.data = NULL;
        hash_tick(st);

    } else {
        /* The current block we have is intended to be data. */

        assert(st->pending_hash.data == NULL);

        /* Just hash the data and stash it for the next tick. */
        st->pending_hash = st->hasher(block);
        hash_tick(st);
    }

    return 0;
}
