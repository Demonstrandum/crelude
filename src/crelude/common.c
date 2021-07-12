#include "common.h"
#include "io.h"
#include "utf.h"

#include <assert.h>

#ifndef IMPLEMENTATION

inline u0 *or(const u0 *nullable, const u0 *nonnull)
{
	if (nullable == nil)
		return (u0 *)nonnull;
	return (u0 *)nullable;
}

bool is_zero(imax n)
{ return n == 0; }

bool is_zerof(f64 n)
{ return n == 0.0; }

bool is_zeroed(u0 *ptr, usize width)
{
	umin *xs = (umin *)ptr;
	until (width-- == 0) unless (*xs++ == 0) return false;
	return true;
}

u0 zero(u0 *blk, usize width)
{
	if (blk == nil || width == 0)
		return UNIT;
	umin *bytes = blk;
	until (width-- == 0)
		*bytes++ = 0;
	return UNIT;
}

u0 *emalloc(usize len, usize size)
{
	usize bytes = len * size;
	u0 *m = MALLOC(bytes);
	if (m == nil)
		PANIC("Could not allocate %zu bytes.", bytes);
	zero(m, bytes);
	return m;
}

/* in-place */
u0 reverse(u0 *self, usize width)
{
	umin chunk[width];  //< C99 variable-length array.
	MemArray *arr = self;
	umin *ptr = arr->value;
	usize len = arr->len * width;

	for (usize i = 0, j = len - 1; i < j; ++i, --j) {
		memcpy(  chunk, ptr + i, width);
		memcpy(ptr + i, ptr + j, width);
		memcpy(ptr + j,   chunk, width);
	}

	return UNIT;
}

MemSlice reverse_endianness(MemSlice bytes)
{
	MemSlice copy;
	copy = COPY(bytes);

	for (usize i = 0, j = copy.len - 1; i < j; ++i, --j) {
		umin temp = NTH(copy, i);
		NTH(copy, i) = NTH(copy, j);
		NTH(copy, j) = temp;
	}

	return copy;
}

bool is_little_endian()
{
	uword w = 0x01;
	return *(umin *)&w == 1;
}

u128 big_endian(umin *start, usize bytes)
{
	assert(bytes <= sizeof(u128));
	// Write to offset in word (big endian), then reverse
	// depending on endianess. i.e., [0x32][0xF1] becomes:
	//   [0x00][0x00][0x32][0xF1]  (big endian), or
	//   [0xF1][0x32][0x00][0x00]  (little endian)
	umin mem[sizeof(u128)];
	memset(mem, 0, sizeof(u128));
	memcpy(mem + sizeof(u128) - bytes, start, bytes);

	MemSlice reversed = VIEW(MemSlice, mem, 0, sizeof(u128));
	if (is_little_endian())  // reverse `mem` if little endian.
		reverse(&reversed, 1);

	return *(u128 *)mem;
}

static u0 memswap(umin *a, umin *b, usize bytes)
{
	usize words = bytes / WORD_SIZE;
	usize trail = words * WORD_SIZE;  //< Where left-over bytes start.

	// Swap word sized blocks first.
	for (usize i = 0; i < words; i += WORD_SIZE) {
		uword tmp;
		memcpy( &tmp, a + i, WORD_SIZE);
		memcpy(a + i, b + i, WORD_SIZE);
		memcpy(b + i,  &tmp, WORD_SIZE);
	}
	// Swap remaining bytes, byte-by-byte.
	for (usize i = trail; i < bytes; ++i) {
		umin tmp = a[i];
		a[i] = b[i];
		b[i] = tmp;
	}
}

/// # Swapping memory (on pivot point `b`):
/// ```
///    a       b     c       d
///    |---A---:------B------|
/// == |---A---:--B1-:---B2--|
/// => |---B2--:--B1-:---A---|
/// => |--B1-:---B2--:---A---|  (by same procedure, recursive)
/// == |------B------:---A---|
/// ```
/// When swapping block A and B.
/// Notice the pivot remains the same (b) for each recursive call.
/// (a, b, and c are pointers, and do not change).
///
/// When the pivot point is `c`,
/// i.e. when block A is the larger block, we have:
/// ```
///    a       b     c       d
///    |------A------:---B---|
/// == |---A1--:--A2-:---B---|
/// => |---B---:--A2-:--A1---|
/// => |---B---:--A1---:--A2-|  (by same procedure, recursive)
/// == |---B---:------A------|
/// ```
static u0 swap_blocks(MemSlice block, usize pivot)
{  // Tail-recursion should be optimised here.
	if (pivot == 0 || pivot == block.len)
		return;  // We are already done.

	MemSlice A = SLICE(MemSlice, block, 0, pivot);
	MemSlice B = SLICE(MemSlice, block, pivot, -1);
	MemSlice A1, A2;
	MemSlice B1, B2;

	if (A.len < B.len) {
		B1 = SLICE(MemSlice, B, 0, -A.len - 1);
		B2 = SLICE(MemSlice, B, B1.len, -1);
		assert(A.len == B2.len);

		memswap(A.value, B2.value, A.len);
		B = SLICE(MemSlice, block, 0, B.len);
		swap_blocks(B, pivot);
	} else if (A.len > B.len) {
		A1 = SLICE(MemSlice, A, 0, B.len);
		A2 = SLICE(MemSlice, A, A1.len, -1);
		assert(B.len == A1.len);
		assert(pivot == A1.len + A2.len);

		memswap(B.value, A1.value, B.len);
		A = SLICE(MemSlice, block, B.len, -1);
		swap_blocks(A, pivot - B.len);
	} else {
		// Same length, and thus trivial.
		memswap(A.value, B.value, A.len);
	}
}

u0 swap(u0 *self, usize pivot, usize width)
{
	// Deal with everything in terms of bytes.
	MemSlice blk = *(MemSlice *)self;
	blk.len *= width;
	usize pos = pivot * width;
	swap_blocks(blk, pos);
}

usize resize(u0 *self, usize cap, usize width)
{
	MemArray *arr = self;
	if (arr->cap == cap)
		return arr->cap - arr->len;
	// Lowering the capacity; best done by REALLOC.
	if (cap < arr->cap) {
		arr->value = REALLOC(arr->value, cap * width);
	} else {  // Growing the capacity; must zero bytes.
		umin *new = emalloc(cap, width);
		memcpy(new, arr->value, arr->cap * width);
		FREE(arr->value);
		arr->value = new;
	}

	arr->cap = cap;
	return arr->cap - arr->len;
}

usize grow(u0 *self, usize count, usize width)
{
	MemArray *arr = self;
	usize old_cap = arr->cap;
	usize new_cap = arr->cap;

	if (arr->len + count > arr->cap) {  // reallocate array.
		new_cap = (usize)(arr->cap * ARRAY_REALLOC_FACTOR) + count;
		resize(arr, new_cap, width);
	}

	arr->len += count;
	return new_cap - old_cap;
}

u0 *get(u0 *self, usize index, usize width)
{
	MemArray *arr = self;
	if (index > arr->len) return nil;

	return arr->value + index * width;
}

u0 *set(u0 *self, usize index, const u0 *elem, usize width)
{
	MemArray *arr = self;
	if (index > arr->len || elem == nil) return nil;

	memcpy(arr->value + index * width, elem, width);
	return arr->value + index * width;
}

usize push(u0 *restrict self, const u0 *restrict element, usize width)
{
	if (element == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	umin *elem = (umin *restrict)element;

	usize growth = grow(arr, 1, width);
	memcpy(arr->value + width * (arr->len - 1), elem, width);
	return growth;
}

usize insert(u0 *restrict self, usize index,
             const u0 *restrict element, usize width)
{
	if (element == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	umin *elem = (umin *restrict)element;
	usize growth = grow(arr, 1, width);

	umin *gap = arr->value + width * index;
	// Offset elements down by one, from insertion index,
	// leaving us with a gap for insertion.
	memmove(gap + width, gap, width * (arr->len - 1 - index));
	// Insert element into the empty space.
	memcpy(gap, elem, width);

	return growth;
}

usize extend(u0 *restrict self, const u0 *restrict slice, usize width)
{
	if (slice == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	MemSlice *sub = (MemSlice *restrict)slice;

	if (sub->len == 0) return 0;

	usize end = arr->len;  //< old array length.
	usize growth = grow(arr, sub->len, width);

	memcpy(arr->value + width * end, sub->value, width * sub->len);
	return growth;
}

usize splice(u0 *restrict self, usize index, const u0 *restrict slice, usize width)
{
	if (slice == nil) return 0;

	MemArray *arr = (MemArray *restrict)self;
	MemSlice *sub = (MemSlice *restrict)slice;

	if (sub->len == 0) return 0;

	usize end = arr->len;
	usize growth = grow(arr, sub->len, width);
	umin *gap = arr->value + width * index;
	// Offset elements down by the slice length, from insertion index,
	// leaving us with a `width * sub.len` byte gap for insertion.
	memmove(gap + width * sub->len, gap, width * (end - index));
	// Insert slice into empty space.
	memcpy(gap, sub->value, sub->len * width);

	return growth;
}

GenericSlice cut(u0 *self, usize from, isize upto, usize width)
{
	GenericArray *arr_ptr = self;
	MemArray arr = *(MemArray *)self;

	usize final = upto < 0 ? arr.len + upto : (usize)upto;
	assert(final < arr.len);

	if (from > final) {
		usize tmp = from;
		from = final;
		final = tmp;
	}
	usize pivot = final - from + 1;

	// Deal with bytes instead.
	arr.len *= width;
	arr.cap *= width;
	from *= width;
	final *= width;
	pivot *= width;

	MemSlice tail = SLICE(MemSlice, arr, from, arr.len);
	swap_blocks(tail, pivot);
	tail = SLICE(MemSlice, tail, tail.len - pivot, -1);

	u0 *tail_ptr = &tail;
	GenericSlice trailing = *(GenericSlice *)tail_ptr;
	trailing.len /= width;
	arr_ptr->len -= pivot / width;  // Capacity the same.
	return trailing;  // Return removed slice which is left over at end.
}

u0 *pop(u0 *self, usize width)
{
	MemArray *arr = self;
	assert(arr->len > 0);
	return (u0 *)(arr->value + --arr->len * width);
}

u0 *shift(u0 *self, usize width)
{
	MemSlice *arr = self;
	assert(arr->len > 0);

	swap(self, 1, width);
	return (u0 *)(arr->value + --arr->len * width);
}

usize null(u0 *self, usize width)
{
	GenericArray arr = *(GenericArray *)self;
	usize bytes = arr.cap * width;
	zero(PTR(arr), bytes);
	return bytes;
}

string from_cstring(const byte *cstring)
{
	return VIEW(string, (byte *)cstring, 0, strlen(cstring));
}

bool string_eq(string self, const string other)
{
	unless (self.len == other.len)
		return false;
	else if (self.value == other.value)
		return true;

	foreach (c, self)
		if (c != NTH(other, it.index))
			return false;
	return true;
}

i16 string_cmp(const string self, const string other)
{
	byte *ptr0 = self.value,
	     *ptr1 = other.value;

	if (ptr0 == ptr1) {
		if (self.len == other.len) return 0;
		return self.len > other.len
			? ptr0[other.len] - 0
			: 0 - ptr1[self.len];
	}

	byte c0, c1;
	usize len = min(self.len, other.len);
	for (usize i = 0; i < len; ++i)
		if ((c0 = ptr0[i]) != (c1 = ptr1[i]))
			return c0 - c1;

	if (self.len == other.len) return 0;
	if (self.len == len)
		return 0 - ptr1[len];
	return ptr0[len] - 0;
}

/// `djb2` hash-algo.
u64 hash_bytes(MemSlice mem)
{
	u64 hash = 5381;
	foreach (c, mem)
		hash += c + (hash << 5);

	return hash;
}

u64 hash_string(string str)
{ return hash_bytes(TO_BYTES(str)); }

/// @note LAYOUT DEPENDENT.
static u64 node_hash(const u0 *self, const u0 *node)
{
	const GenericMap *map = self;
	return *(u64 *)((umin *)node + map->hash_offset);
}
/// @note LAYOUT DEPENDENT.
static u0 *node_key(const u0 *self, const u0 *node)
{
	const GenericMap *map = self;
	return (umin *)node + map->key_offset;
}
/// @note LAYOUT DEPENDENT.
static u0 *node_value(const u0 *self, const u0 *node)
{
	const GenericMap *map = self;
	return (umin *)node + map->value_offset;
}
/// @note LAYOUT DEPENDENT.
static u0 *node_next(const u0 *self, const u0 *node)
{
	const GenericMap *map = self;
	return (umin *)node + map->next_offset;
}

/// Get bucket index given key.
static usize bucket_index(const u0 *self, u64 hash)
{
	const GenericMap *map = self;
	return (usize)hash % map->buckets.cap;
}

// TODO: right now we check proper equality, maybe just use the hash,
//       and don't worry about hash-collisions?  `u64` is quite large after all.
static bool key_eq(const u0 *self, const u0 *key0, const u0 *key1)
{
	if (key0 == key1) return true;
	const GenericMap *map = self;

	switch (map->key_type) {
	case HKT_STRING:
	case HKT_MEM_SLICE:;
		const string *s0 = key0, *s1 = key1;
		return string_eq(*s0, *s1);
	case HKT_RUNIC:;
		const runic *r0 = key0, *r1 = key1;
		MemSlice _m0 = TO_BYTES(*r0), _m1 = TO_BYTES(*r1);
		MemSlice *m0 = &_m0, *m1 = &_m1;
		return string_eq(*(string *)m0, *(string *)m1);
	case HKT_CSTRING:
		return 0 == strcmp(*(byte **)key0, *(byte **)key1);
	case HKT_RAW_BYTES:
		return 0 == memcmp(key0, key1, map->key_size);
	}

	return PANIC("Improper hash-map key_type."), false;
}

/// @note LAYOUT DEPENDENT.
usize init_hashnode(u0 *node, const u0 *_map, u64 hash, const u0 *key, const u0 *value)
{
	GenericMap *map = (u0 *)_map;
	umin *ptr = node;

	memcpy(ptr + map-> hash_offset, &hash, sizeof(u64));
	memcpy(ptr + map->  key_offset,   key, map->key_size);
	memcpy(ptr + map->value_offset, value, map->value_size);
	memset(ptr + map-> next_offset,     0, sizeof(u0 *));  //< NULL-pointing.

	return map->node_size;
}

u0 associate(u0 *self, const u0 *key, const u0 *value)
{
	GenericMap *map = self;
	const usize NODE_SIZE = map->node_size;

	u64 hash = map->hasher(key, map->key_size);
	usize index = bucket_index(map, hash);

	u0 *head = (umin *)PTR(map->buckets) + index * NODE_SIZE;
	u0 *node = head;  /*< type of `hashnode(K, V)`. */
	u0 *last = nil;
	// Walk up node-chain trying to find if key is already present.
	until (node == nil || node_hash(map, node) == 0) {  // zero-hash = unpopulated.
		if (key_eq(map, node_key(map, node), key)) {
			// Found node with equal key, so rewrite value.
			memcpy(node_value(map, node), value, map->value_size);
			return UNIT;
		}
		last = node;
		node = *(u0 **)node_next(map, node);
	}
	// Otherwise, insert the new key-value pair into the chain.
	assert(node == nil || node_hash(map, node) == 0);
	++map->len;

	u0 *new = emalloc(1, NODE_SIZE);
	init_hashnode(new, map, hash, key, value);

	if (last == nil) {  // i.e. chain hasn't started.
		assert(node == head);
		memcpy(head, new, NODE_SIZE);
		grow(&map->buckets, 1, NODE_SIZE);
		FREE(new);
	} else {  // otherwise, make the node part of the linked list.
		u0 **last_next = node_next(map, last);
		assert(*last_next == nil);
		*last_next = new;
	}

	const f64 LOAD_FACTOR = (f64)map->len / map->buckets.cap;
	if (LOAD_FACTOR < HASHMAP_LOAD_THRESHOLD)
		return UNIT;
	// ^ if load factor (entries per potential bucket) gets over ~85%,
	// we should ~double it in capacity, preventing the linked lists
	// from getting to long, and lookup time too slow.

	/// re-index whole array.  Growing the array means hash-values
	/// are modulo'd to different values, and so we need to re-arrange.
	GenericArray snodes = AMAKE(umin[NODE_SIZE], map->len);
	for (usize i = 0; i < map->buckets.cap; ++i) {
		u0 *snode = (umin *)PTR(map->buckets) + i * NODE_SIZE;
		if (node_hash(map, snode) == 0) continue;
		bool first = true;
		until (snode == nil) {
			u0 **next_field = node_next(map, snode);
			u0 *next = *next_field;
			*next_field = nil;
			push(&snodes, snode, NODE_SIZE);
			snode = next;
			if (first) { first = false; continue; }
			FREE(snode);  // we have a copy.
		}
	}
	// copied `snodes` (base/bucket nodes), now blank out the buckets array,
	// and rewrite it with new and correct indices.
	resize(&map->buckets,
	       CEIL(usize, HASHMAP_GROWTH_FACTOR
	         * (LOAD_FACTOR / HASHMAP_LOAD_THRESHOLD)
	         * map->buckets.cap),
		   NODE_SIZE);
	null(&map->buckets, NODE_SIZE);
	// repopulate.
	umin *bucket = (u0 *)PTR(map->buckets);
	for (usize i = 0; i < snodes.len; ++i) {
		u0 *snode = get(&snodes, i, NODE_SIZE);
		u64 shash = node_hash(map, snode);
		usize sindex = bucket_index(map, shash);
		u0 *bnode = bucket + sindex * NODE_SIZE;
		// copy directly into bucket array.
		if (node_hash(map, bnode) == 0) {
			memcpy(bnode, snode, NODE_SIZE);
			continue;
		}
		// otherwise, append to chain.
		until (*(u0 **)node_next(map, bnode) == nil)
			bnode = *(u0 **)node_next(map, bnode);
		assert(bnode != nil);
		// put `snode` on heap, and link up pointer to it.
		u0 *new_snode = emalloc(1, NODE_SIZE);
		memcpy(new_snode, snode, NODE_SIZE);
		u0 **next_field = node_next(map, bnode);
		*next_field = new_snode;
	}
	FREE_INSIDE(snodes);

	return UNIT;
}

u0 *lookup(u0 *self, const u0 *key)
{
	GenericMap *map = self;
	u64 hash = map->hasher(key, map->key_size);
	usize index = bucket_index(map, hash);

	u0 *head = (umin *)PTR(map->buckets) + index * map->node_size;
	// search hashnode-chain.
	until (head == nil || node_hash(map, head) == 0) {
		if (key_eq(map, node_key(map, head), key))
			return node_value(map, head);
		head = *(u0 **)node_next(map, head);
	}

	return nil;
}

bool drop(u0 *self, const u0 *key)
{
	GenericMap *map = self;
	u64 hash = map->hasher(key, map->key_size);
	usize index = bucket_index(map, hash);

	u0 *head = (umin *)PTR(map->buckets) + index * map->node_size;
	u0 *node = head;
	u0 *last = nil;
	// search hasnode-chain.
	until (node == nil || node_hash(map, node) == 0) {
		if (key_eq(map, node_key(map, node), key))
			break;
		last = node;
		node = *(u0 **)node_next(map, node);
	}

	if (node == nil || node_hash(map, node) == 0)
		// i.e. loop did not break, and
		// so the key does not exist.
		return false;

	if (node == head) {  // delete from bucket array directly.
		assert(last == nil);
		u0 *next = *(u0 **)node_next(map, node);
		if (next == nil) {
			zero(node, map->node_size);
			--map->buckets.len;
		} else {
			// otherwise, we need to copy the node into the bucket array,
			// overwriting the node that we are deleting.
			memcpy(node, next, map->node_size);
			FREE(next);  //< it is now in the base of the bucket array.
		}
	} else {
		// otherwise, re-order the pointers and free the node.
		assert(last != nil);
		u0 **last_next_field = node_next(map, last);
		u0 **node_next_field = node_next(map, node);
		*last_next_field = *node_next_field;
		FREE(node);
	}

	--map->len;
	return true;
}

GenericSlice get_keys(u0 *self)
{
	GenericMap *map = self;
	GenericSlice ks = {
		.len = map->len,
		.value = emalloc(map->len, map->key_size)
	};

	usize j = 0;
	for (usize i = 0; i < map->buckets.cap; ++i) {
		u0 *node = (umin *)PTR(map->buckets) + i * map->node_size;
		if (node_hash(map, node) == 0) continue;

		until (node == nil) {
			set(&ks, j++, node_key(map, node), map->key_size);
			node = *(u0 **)node_next(map, node);
		}
	}
	assert(j == map->len);
	return ks;
}

bool has_key(u0 *self, u0 *key)
{
	GenericMap *map = self;
	for (usize i = 0; i < map->buckets.cap; ++i) {
		u0 *node = (umin *)PTR(map->buckets) + i * map->node_size;
		if (node_hash(map, node) == 0) continue;

		until (node == nil) {
			if (key_eq(map, node_key(map, node), key))
				return true;
			node = *(u0 **)node_next(map, node);
		}
	}
	return false;
}

u0 empty_map(u0 *self)
{
	GenericMap *map = self;
	for (usize i = 0; i < map->buckets.cap; ++i) {
		u0 *node = (umin *)PTR(map->buckets) + i * map->node_size;
		if (node_hash(map, node) == 0) continue;

		node = *(u0 **)node_next(map, node);
		until (node == nil) {
			u0 *next = *(u0 **)node_next(map, node);
			FREE(node);
			node = next;
		}
	}
	zero(PTR(map->buckets), map->node_size * map->buckets.cap);
	return UNIT;
}

bool is_empty_map(u0 *self)
{
	GenericMap *map = self;
	if (PTR(map->buckets) == nil) return true;

	for (usize i = 0; i < map->buckets.cap; ++i) {
		u0 *node = (umin *)PTR(map->buckets) + i * map->node_size;
		if (node_hash(map, node) != 0) return false;
	}

	return true;
}

u0 free_map(u0 *self)
{
	GenericMap *map = self;
	empty_map(map);
	FREE_INSIDE(map->buckets);
	map->buckets.value = nil;
	return UNIT;
}

u0 dump_hashmap(u0 *self, byte *key_fmt, byte *value_fmt)
{
	GenericMap *map = self;
	const usize NODE_SIZE = map->node_size;
	umin *buckets = (u0 *)PTR(map->buckets);
	eprintln("entries:     %zu", map->len);
	eprintln("buckets.cap: %zu", map->buckets.cap);
	eprintln("buckets.len: %zu", map->buckets.len);

	for (usize i = 0; i < map->buckets.cap; ++i) {
		u0 *node = buckets + i * NODE_SIZE;
		u64 bhash = node_hash(map, node);

		eprint("| %02zu |", i);
		if (bhash == 0) {
			eprintln(" -x-");
			continue;
		}

		until (node == nil) {
			struct { umin _[16]; } key = { 0 }, value = { 0 };
			memcpy(&key, node_key(map, node), map->key_size);
			memcpy(&value, node_value(map, node), map->value_size);
			u64 hash = node_hash(map, node);
			string formatter = sprint(" -> [%s (%06llX): %s]",
									  key_fmt, hash, value_fmt);
			eprint(PTR(formatter), key, value);
			node = *(u0 **)node_next(map, node);
		}
		eprint("\n");
	}
}

#endif

