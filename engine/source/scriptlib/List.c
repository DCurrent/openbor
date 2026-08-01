/*
 * List library.
 * 90% 2011 anallyst
 * 10% unknown author
 *
 * the whole smart stuff going on here was written by anallyst.
 * at the time anallyst touched this code, this was a very simple single linked list.
 * double linked list functionality, index hash and string hash functionality by anallyst.
 *
 */

#include "List.h"
#include <assert.h>

#ifdef DEBUG
void chklist(List *list)
{
    assert(list);
    assert(list->initdone == 1); // method called on uninitialised list
}
#endif

#ifdef USE_STRING_HASHES

/*
 * Caskey, Damon V.
 * 2026-07-29 - Original implementation 
 * by Anallyst, circa 2011.
 *
 * Returns a 64-bit string hash using the 
 * FNV-1a algorithm. The previous implementation 
 * used an additive hash and returned an 8-bit 
 * bucket index.
 */
static uint64_t strhash(const char *s) {
    uint64_t hash = UINT64_C(14695981039346656037);

    while(*s) {
        hash ^= (unsigned char)*s++;
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

/*
 * Caskey, Damon V.
 * 2026-07-29 - Reworked original implementation
 * by Anallyst, circa 2011.
 *
 * Adds a named node to the list's string hash
 * index. Uses cached 64-bit FNV-1a hashes and
 * bucket-based collision handling.
 */
void List_AddHash(List *list, Node *node) {
#ifdef DEBUG
    chklist((List *)list);
#endif

    assert(node);

    if(!node->name) {
        node->name_hash = 0;
        return;
    }

    /*
     * Allocate the bucket directory on the 
     * first named insertion. Lists containing 
     * only unnamed entries never pay this cost.
     */
    if(!list->buckets) {
        list->buckets = calloc(LIST_STRING_HASH_BUCKET_COUNT, sizeof(*list->buckets));

        assert(list->buckets != NULL);
    }

    /*
     * Cache the full hash on the node. The low 
     * bits select the bucket, while lookups compare 
     * the complete cached hash.
     */
    const uint64_t hash = strhash(node->name);
    node->name_hash = hash;
    
    const size_t hash_bucket = hash & LIST_STRING_HASH_BUCKET_MASK;
    Bucket *bucket = list->buckets[hash_bucket];

    /*
     * Allocate individual buckets only when 
     * they receive an entry.
     */
    if(!bucket) {
        bucket = calloc(1, sizeof(*bucket));
        assert(bucket != NULL);

        bucket->nodes = calloc(LIST_STRING_HASH_BUCKET_INITIAL_SIZE, sizeof(*bucket->nodes));

        assert(bucket->nodes != NULL);
        bucket->size = LIST_STRING_HASH_BUCKET_INITIAL_SIZE;
        list->buckets[hash_bucket] = bucket;
    }

    /*
     * Grow unusually crowded buckets geometrically.
     */
    const size_t capacity = bucket->size;

    assert(bucket->used <= capacity);

    if(bucket->used == capacity) {

        Node **expanded_nodes = realloc(bucket->nodes, sizeof(*bucket->nodes) * (capacity * 2));
        assert(expanded_nodes != NULL);

        bucket->nodes = expanded_nodes;
        bucket->size = capacity * 2;
    }

    bucket->nodes[bucket->used] = node;
    bucket->used++;
}

/*
 * Caskey, Damon V.
 * 2026-07-29 - Reworked original implementation
 * by Anallyst, circa 2011.
 *
 * Removes a named node from the list's string hash
 * index. Compacts the bucket array to keep occupied
 * entries contiguous and preserve insertion order.
 * Empty bucket storage is retained for later reuse.
 */
void List_RemoveHash(List *list, Node *node) {
#ifdef DEBUG
    chklist((List *)list);
#endif

    assert(node);

    /*
     * Unnamed nodes are never added to the string
     * hash index and require no removal.
     */
    if(!node->name) {
        return;
    }

    /*
     * Use the cached hash to locate the bucket without
     * hashing the node's name again.
     */
    const size_t hash_bucket = node->name_hash & LIST_STRING_HASH_BUCKET_MASK;

    assert(list->buckets != NULL);

    Bucket *bucket = list->buckets[hash_bucket];

    assert(bucket != NULL);
    assert(bucket->used > 0);

    /*
     * Locate the exact node by pointer identity. Several
     * nodes may occupy the same bucket or use the same name.
     */
    for(size_t i = 0; i < bucket->used; i++) {
        if(node == bucket->nodes[i]) {
            /*
             * Reduce the occupied count, then move subsequent
             * entries down to close the removed node's slot.
             * memmove() safely handles the overlapping ranges.
             */
            bucket->used--;

            if(i < bucket->used) {
                memmove(bucket->nodes + i, bucket->nodes + i + 1, sizeof(*bucket->nodes) * (bucket->used - i));
            }

            /*
             * Clear the now-unused final slot. Bucket storage
             * remains allocated so later insertions can reuse it.
             */
            bucket->nodes[bucket->used] = NULL;
            return;
        }
    }

    /*
     * Reaching this point means the linked list and its
     * string hash index have become inconsistent.
     */
    assert(0 && "node missing from string hash bucket");
}

/*
 * Caskey, Damon V.
 * 2026-07-29 - Reworked original implementation
 * by Anallyst, circa 2011.
 *
 * Releases all memory owned by the list's string
 * hash index. Frees each bucket's node-pointer
 * array, each bucket, and the bucket directory.
 *
 * This function does not free the referenced Nodes,
 * their names, or their values. Those remain under
 * the ownership of the parent List.
 */
void List_FreeHashes(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif

    /*
     * The bucket directory is allocated lazily, so
     * lists without named entries have nothing to free.
     */
    if(!list->buckets) {
        return;
    }

    /*
     * Release each allocated bucket. Unused directory
     * positions remain NULL and require no cleanup.
     */
    for(size_t i = 0; i < LIST_STRING_HASH_BUCKET_COUNT; i++)  {
        Bucket *bucket = list->buckets[i];

        if(bucket) {
            free(bucket->nodes);
            free(bucket);
        }
    }

    /*
     * Release the bucket directory and clear its pointer
     * to prevent stale access or accidental double-free.
     */
    free(list->buckets);
    list->buckets = NULL;
}

/*
 * Caskey, Damon V.
 * 2026-07-29 - Reworked original implementation
 * by Anallyst, circa 2011.
 *
 * Rebuilds the list's complete string hash index
 * from its linked-list nodes. Any existing hash
 * index is discarded before reconstruction.
 *
 * Currently used after copying a List, but safe
 * whenever the index requires a complete rebuild.
 */
void List_CreateHashes(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif

    /*
     * Discard the existing index before rebuilding.
     * This prevents duplicate node pointers when part
     * of the index was already populated by insertion.
     */
    List_FreeHashes(list);

    /*
     * Walk the authoritative linked list and recreate
     * the index. List_AddHash() recalculates and caches
     * each named node's hash while skipping unnamed nodes.
     */
    Node *node = list->first;

    while(node) {
        List_AddHash(list, node);
        node = node->next;
    }
}
#endif

#ifdef USE_INDEX
unsigned char ptrhash(void *value)
{
    size_t tmp = (size_t) value;
    tmp >>= 4;
    return tmp % 256;
}

/* add a single node to the index list */
void List_AddIndex(List *list, Node *node, size_t index)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    unsigned char h;
    size_t save;

    assert(node);

    if (!list->mindices)
    {
        list->mindices = calloc(1, sizeof(LIndex *) * 256);
    }

    h = ptrhash(node->value);
    if (!list->mindices[h])
    {
        list->mindices[h] = calloc(1, sizeof(LIndex));
        list->mindices[h]->nodes = calloc(1, sizeof(Node *) * 8);
        assert(list->mindices[h]->nodes != NULL);
        list->mindices[h]->indices = calloc(1, sizeof(ptrdiff_t) * 8);
        assert(list->mindices[h]->indices != NULL);
        list->mindices[h]->size = 8;
    }

    save = list->mindices[h]->size;
    assert(list->mindices[h]->used <= save);
    if (list->mindices[h]->used == save)
    {
        list->mindices[h]->nodes = realloc(list->mindices[h]->nodes, sizeof(Node *) * (save * 2));
        assert(list->mindices[h]->nodes != NULL);
        list->mindices[h]->indices = realloc(list->mindices[h]->indices, sizeof(ptrdiff_t) * (save * 2));
        assert(list->mindices[h]->indices != NULL);
        list->mindices[h]->size = save * 2;
    }

    list->mindices[h]->nodes[list->mindices[h]->used] = node;
    list->mindices[h]->indices[list->mindices[h]->used] = index;
    list->mindices[h]->used++;
}

/* removes the last element from the index list
   only use on fully indexed list */
void List_RemoveLastIndex(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    unsigned char h;
    assert(list->last);
    assert(list->current);
    assert(list->current == list->last);
    h = ptrhash(list->last->value);
    assert(list->mindices[h]);
    assert(list->mindices[h]->used > 0);
    list->mindices[h]->used--; // it would be wrong to do this to a random element, but it's ok for the last one, since it was added as last
    list->mindices[h]->nodes[list->mindices[h]->used] = NULL;
    list->mindices[h]->indices[list->mindices[h]->used] = 0;
}

/* build indices for entire list
   the indices will be destroyed whenever an element is either
   inserted or removed from the list,
   except if it was the last node/inserted after the last node */
void List_CreateIndices(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    Node *n = list->first;
    size_t index = 0;
    while(n)
    {
        List_AddIndex(list, n, index);
        index++;
        n = n->next;
    }
}

/* free everything related to the index list
   usually you dont have to do it manually, since its called by List_Clear
   but it won't hurt either */
void List_FreeIndices(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    int i;
    if(!list->mindices)
    {
        return;
    }
    for (i = 0; i < 256; i++)
    {
        if(list->mindices[i])
        {
            free(list->mindices[i]->indices);
            free(list->mindices[i]->nodes);
            free(list->mindices[i]);
        }
    }
    free(list->mindices);
    list->mindices = NULL;
}
#endif

int List_GotoLast(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef LIST_DEBUG
    printf("List_Last %p\n", list);
#endif
    if(list->size)
    {
        list->current = list->last;
    }
    else
    {
        return 0;
    }
    return 1;
}

int List_GotoFirst(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef LIST_DEBUG
    printf("List_First %p\n", list);
#endif
    if(list->size)
    {
        list->current = list->first;
    }
    else
    {
        return 0;
    }
    return 1;
}

Node *List_GetCurrentNode(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    return list == NULL ? NULL : list->current;
}

void List_SetCurrent(List *list, Node *current)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    if (list)
    {
        list->current = current;
    }
}


void Node_Clear(Node *node)
{
    if(!node)
    {
        return;
    }
    if(node->name)
    {
        free((void *)node->name);
    }
}

void List_Init(List *list)
{
#ifdef LIST_DEBUG
    printf("List_Init %p\n", list);
#endif
    list->first = list->current = list->last = NULL;
    list->size = list->index = 0;
    list->solidlist = NULL;
#ifdef USE_INDEX
    list->mindices = NULL;
#endif
#ifdef USE_STRING_HASHES
    list->buckets = NULL;
#endif
#ifdef DEBUG
    list->initdone = 1;
#endif
}

/*
* Converts a linked List into a contiguous array of
* stored value pointers.
*
* Returns true when the list is empty or solidification
* succeeds. Returns false when the list is invalid,
* allocation size overflows, or allocation fails.
*
* Allocation completes before the linked nodes are
* removed. Failure therefore leaves the original list
* and any previous solid list intact.
*/
bool List_Solidify(List *list) {

    int i = 0;
    void **solidlist;

    if(!list || list->size < 0) {
        return false;
    }

#ifdef DEBUG
    chklist(list);
#endif

#ifdef LIST_DEBUG
    printf("List_Solidify %p\n", list);
#endif

    if(list->size == 0) {
        free(list->solidlist);
        list->solidlist = NULL;
        return true;
    }

    /*
    * A solidified list retains its logical size after
    * releasing the linked nodes. Avoid replacing its
    * populated pointer table with uninitialized storage.
    */
    if(!list->first) {
        return list->solidlist != NULL;
    }

    if((size_t)list->size > SIZE_MAX / sizeof(*solidlist)) {
        return false;
    }

    const size_t allocation_size = sizeof(*solidlist) * (size_t)list->size;

    solidlist = malloc(allocation_size);

    if(!solidlist) {
        return false;
    }

    free(list->solidlist);
    list->solidlist = solidlist;

    const int saved_size = list->size;

    /*
    * Copy each stored value pointer into contiguous
    * storage. List_Remove() releases the corresponding
    * node while preserving ownership of its value.
    */
    List_GotoFirst(list);

    while(list->current) {
        assert(i < saved_size);

        list->solidlist[i++] =
            list->current->value;

        List_Remove(list);
    }

    assert(i == saved_size);

    /*
    * Solidification removes the nodes but preserves the
    * logical number of stored values for consumers that
    * query List_GetSize().
    */
    list->size = saved_size;

#ifdef LIST_DEBUG
    printf("solidlist of %p:\n", list);
#endif

    list->first = NULL;
    list->current = NULL;
    list->last = NULL;
    list->index = 0;

    /*
    * Linked-node indexes are no longer applicable after
    * the nodes have been removed.
    */
#ifdef USE_INDEX
    if(list->mindices) {
        List_FreeIndices(list);
    }
#endif

#ifdef USE_STRING_HASHES
    List_FreeHashes(list);
#endif

    return true;
}


int List_GetNodeIndex(List *list, Node *node)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    int i;
    Node *n;
#ifdef USE_INDEX
    unsigned char h;
    if(list->mindices)
    {
        h = ptrhash(node->value);
        assert(list->mindices[h]);
        for(i = 0; i < list->mindices[h]->used; i++)
        {
            //assert(list->mindices[h]->nodes[i]); gets overwritten by update with NULL
            if(list->mindices[h]->nodes[i] && list->mindices[h]->nodes[i] == node)
            {
                return list->mindices[h]->indices[i];
            }
        }
        return -1;
    }
    else
#endif
    {
        n = list->first;
        i = 0;
        while(n)
        {
            if(n == node)
            {
                return i;
            }
            i++;
            n = n->next;
        }
        return -1;
    }
}

int List_GetIndex(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    return List_GetNodeIndex(list, list->current);
}

/*
 * Caskey, Damon V.
 * 2026-07-29 - Reworked original implementation
 * by Anallyst, circa 2011.
 *
 * Copies the source List's node structure into an
 * initialized destination List, replacing any existing
 * destination contents.
 *
 * Node structures and names are duplicated. Value
 * pointers remain shared. The source and destination
 * must be distinct initialized Lists.
 */
void List_Copy(List *listdest, const List *listsrc) {
#ifdef DEBUG
    chklist((List *)listsrc);
    chklist(listdest);
#endif
#ifdef LIST_DEBUG
    printf("List_Copy %p %p\n", listsrc, listdest);
#endif

    /*
     * Self-copy would destroy the source when the
     * destination is cleared.
     */
    assert(listdest != listsrc);

    if(listdest == listsrc) {
        return;
    }

    /*
     * Release the destination's existing node and index
     * storage, then return it to an initialized state.
     */
    List_Clear(listdest);

    const Node *source_node = listsrc->first;
    Node *copied_current = NULL;

    /*
     * Insert every node through the normal insertion path.
     * This copies names, maintains links, and creates the
     * string hash index without requiring a later rebuild.
     */
    while(source_node) {
        List_InsertAfter(listdest, source_node->value, source_node->name);

        if(source_node == listsrc->current) {
            copied_current = listdest->current;
        }

        source_node = source_node->next;
    }

    /*
     * A populated source is expected to have a valid
     * current node.
     */
    if(listsrc->first) {
        assert(copied_current != NULL);
        listdest->current = copied_current;
    }

#ifdef USE_INDEX
    /*
     * Recreate the optional value index only when the
     * source had one.
     */
    if(listsrc->mindices) {
        List_CreateIndices(listdest);
    }
#endif
}

void List_Clear(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef LIST_DEBUG
    printf("List_clear %p \n", list);
#endif

    //Delete all the Nodes in the list.
    Node *nptr = list->first;
    list->current = list->first;

    while(list->current)
    {
        list->current = list->current->next;
        Node_Clear(nptr);
        free(nptr);
        nptr = list->current;
    }
    if(list->solidlist)
    {
        free(list->solidlist);
        list->solidlist = NULL;
    }

#ifdef USE_INDEX
    if(list->mindices)
    {
        List_FreeIndices(list);
    }
#endif

#ifdef USE_STRING_HASHES
    List_FreeHashes(list);
#endif

    List_Init(list);
}

//Insertion functions
void List_InsertBefore(List *list, void *e, const char *theName)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef LIST_DEBUG
    printf("List_InsertBefore %p %s\n", list, theName ? theName : "no-name");
#endif
#ifdef USE_INDEX
    if (list->mindices)
    {
        List_FreeIndices(list);    // inserting something before something else destroys our indices list.
    }
#endif

    //Construct a new Node
    Node *nptr = (Node *)malloc(sizeof(Node));
    assert(nptr != NULL);

    nptr->value = e;
    nptr->name = NAME(theName);
    nptr->name_hash = 0;

#ifdef USE_STRING_HASHES
    List_AddHash(list, nptr);
#endif

    if (list->size == 0)
    {
        nptr->next = NULL;
        nptr->prev = NULL;
        list->current = list->first = list->last = nptr;
    }
    else
    {
        nptr->next = list->current;
        nptr->prev = list->current->prev;
        if (list->current->prev != NULL)
        {
            list->current->prev->next = nptr;
        }
        list->current->prev = nptr;
        if (list->current == list->first)
        {
            list->first = nptr;
        }
        list->current = nptr;
    }
    list->size++;
}

void List_InsertAfter(List *list, void *e, const char *theName)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef LIST_DEBUG
    printf("List_InsertAfter %p %s\n", list, theName ? theName : "no-name");
#endif
#ifdef USE_INDEX
    int doIndex = 0;
    if (list->mindices)
    {
        if(list->current != list->last)
        {
            List_FreeIndices(list);    // inserting something in the middle of something else destroys our indices list.
        }
        else
        {
            doIndex = 1;
        }
    }
#endif

    //Construct a new Node and fill it with the appropriate value
    Node *nptr = (Node *)malloc(sizeof(Node));

    assert(nptr != NULL);
    nptr->value = e;
    nptr->name = NAME(theName);
    nptr->name_hash = 0;

#ifdef USE_STRING_HASHES
    List_AddHash(list, nptr);
#endif

    if (list->size == 0)
    {
        nptr->prev = NULL;
        nptr->next = NULL;
        list->current = list->first = list->last = nptr;
    }
    else
    {
        nptr->next = list->current->next;
        nptr->prev = list->current;
        if (list->current->next != NULL)
        {
            list->current->next->prev = nptr;
        }
        list->current->next = nptr;
        if (list->current == list->last)
        {
            list->last = nptr;
        }
        list->current = nptr;
    }
#ifdef USE_INDEX
    if (doIndex)
    {
        List_AddIndex(list, list->current, list->size);
    }
#endif
    list->size++;
}

//removes the current node and sets current to next, if applicable
void List_Remove(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    Node *nptr;
#ifdef LIST_DEBUG
    printf("List_Remove %p\n", list);
#endif
    if (list->size == 0)
    {
        //OutputDebugString("Attempted to remove from empty list.\n");
        return;
    }
    else if (list->size == 1)
    {
#ifdef USE_STRING_HASHES
        List_RemoveHash(list, list->current);
#endif
        Node_Clear(list->current);
        free(list->current);
        list->first = list->current = list->last = NULL;
#ifdef USE_INDEX
        List_FreeIndices(list);
#endif
    }
    else
    {
#ifdef USE_INDEX
        if(list->mindices)
        {
            if (list->current != list->last)
            {
                List_FreeIndices(list);    // removing something before something else destroys our indices list.
            }
            else
            {
                List_RemoveLastIndex(list);
            }
        }
#endif

        if(list->current->prev != NULL)
        {
            list->current->prev->next = list->current->next;
        }
        if(list->current->next)
        {
            list->current->next->prev = list->current->prev;
        }
        if (list->current == list->last)
        {
            nptr = list->current->prev;
        }
        else
        {
            nptr = list->current->next;
        }

#ifdef USE_STRING_HASHES
        List_RemoveHash(list, list->current);
#endif

        Node_Clear(list->current);
        free(list->current);

        if (list->current == list->last)
        {
            list->last = nptr;
        }
        if (list->current == list->first)
        {
            list->first = nptr;
        }
        list->current = nptr;
    }
    list->size--;
}

int List_GotoNext(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef LIST_DEBUG
    printf("List_Next %p\n", list);
#endif
    if (list->current != list->last)
    {
        list->current = list->current->next;
    }
    else
    {
        return 0;
    }
    return 1;
}

int List_GotoPrevious(List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef LIST_DEBUG
    printf("List_Prev %p\n", list);
#endif
    if (list->current->prev)
    {
        list->current = list->current->prev;
    }
    else
    {
        return 0;
    }
    return 1;
}

void *List_Retrieve(const List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    if (list->current)
    {
        return list->current->value;
    }
    else
    {
        return NULL;
    }
}

void *List_GetFirst(const List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    if (list->first)
    {
        return list->first->value;
    }
    else
    {
        return NULL;
    }
}

void *List_GetLast(const List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    if (list->last)
    {
        return list->last->value;
    }
    else
    {
        return NULL;
    }
}

void List_Update(List *list, void *e)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
#ifdef USE_INDEX
    unsigned char h;
    ptrdiff_t save, i;
    if(list->mindices)
    {
        h = ptrhash(list->current->value);
        assert(list->mindices[h]);
        for(i = 0; i < list->mindices[h]->used; i++)
        {
            if(list->mindices[h]->nodes[i] == list->current)
            {
                list->mindices[h]->nodes[i] = NULL;
                save = list->mindices[h]->indices[i];
                list->mindices[h]->indices[i] = -1;
                list->current->value = e;
                List_AddIndex(list, list->current, save);
                break;
            }
        }
    }
    else
#endif
        if (list->size != 0)
        {
            list->current->value = e;
        }
}

/* returns the node that contains e, or NULL, if not found */
Node *List_GetNodeByValue(List *list, void *e)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    Node *n;
#ifdef USE_INDEX
    unsigned char h;
    ptrdiff_t i;
    if(list->mindices)
    {
        h = ptrhash(e);
        if (!list->mindices[h])
        {
            return NULL;
        }
        for(i = 0; i < list->mindices[h]->used; i++)
        {
            //assert(list->indices[h]->nodes[i]); gets overwritten by update with NULL
            if(list->mindices[h]->nodes[i] && list->mindices[h]->nodes[i]->value == e)
            {
                //gotcha
                return list->mindices[h]->nodes[i];
            }
        }
        return NULL;
    }
    else
#endif
    {
        n = list->first;
        while (n && (n->value != e))
        {
            n = n->next;
        }
        return n;
    }
}


/* SIDE EFFECT: Moves list->current to found entity.
use List_Contains if you dont like that.
*/
int List_Includes(List *list, void *e)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    Node *n = List_GetNodeByValue(list, e);
    if (n)
    {
        list->current = n;
        return 1;
    }
    return 0;
}

/* returns the first node of which name is equal to theName */
Node *List_GetNodeByName(List *list, const char *name)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    Node *nptr;
    if (!name)
    {
        return NULL;
    }

#ifdef USE_STRING_HASHES
    uint64_t hash;
    size_t i;
    size_t h;

    hash = strhash(name);
    h = hash & LIST_STRING_HASH_BUCKET_MASK;
    if (!list->buckets || !list->buckets[h])
    {
        return 0;
    }
    for(i = 0; i < list->buckets[h]->used; i++)
    {
        nptr = list->buckets[h]->nodes[i];
        if(nptr
           && nptr->name_hash == hash
           && strcmp(name, nptr->name) == 0)
        {
            return nptr;
        }
    }
    return NULL;
#else
    nptr = list->first;

    while (nptr)
    {
        if (nptr->name)
        {
            if (strcmp( nptr->name, name ) == 0)
            {
                return nptr;
            }
        }
        nptr = nptr->next;
    }
    return NULL;
#endif

}

/* SIDE EFFECTS: sets list->current to the first found node */
bool List_FindByName(List *list, const char *name)
{
#ifdef DEBUG
    chklist((List *)list);
#endif

    Node *n = List_GetNodeByName(list, name);
    if (n) {
        list->current = n;
        return true;
    }
    return false;
}

char *List_GetName(const List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    if (list->size != 0)
    {
        return list->current->name;
    }
    else
    {
        return NULL;
    }
}

void List_Reset(List *list)
{
#ifdef DEBUG
    chklist(list);
#endif
#ifdef LIST_DEBUG
    printf("List_Reset %p\n", list);
#endif
    list->current = list->first;
}

int List_GetSize(const List *list)
{
#ifdef DEBUG
    chklist((List *)list);
#endif
    return list->size;
}


