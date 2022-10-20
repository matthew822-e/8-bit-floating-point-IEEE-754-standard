/* A non-FP based C Implementation of a Hashmap using Separate Chaining
 * Copyright Kevin Andrea - 2022
 * <String,void *> Entries
 * Only a single instance of the Hashmap is supported.
 * The Value *must* be Dynamically Allocated and WILL be freed on clear/remove.
 * - The function to free the value is passed in on creation of the hashmap.
 * The Value will NOT be freed or removed from the Hashmap on GET.  (Pointer to Value)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

// Structure used for Separate Chaining 
typedef struct carrier_struct {
  char key[MAX_KEY_SIZE];         // The key is a String
  void *value;                    // Put anything in here you like!
  struct carrier_struct *next;    // Doubly-Linked List for Sep. Chaining
  struct carrier_struct *prev;    // Doubly-Linked List for Sep. Chaining
} Carrier_t;

// Main Hashmap Structure
typedef struct hashmap_struct {
  int size;                       // Number of Keys in HashMap
  int capacity;                   // Number of Indices in HashMap
  int base_capacity;              // Base Number of Indices in HashMap (on Clear)
  void (*free_value)(void *value);// Function to call to free the Value
  struct carrier_struct **table;  // 1D Array of Carrier Pointers
} Hashmap_t;

// Private Globals (Private to this Source File Only)
static Hashmap_t *hashmap = NULL;
enum hashmap_clear_options { Hashmap_NoFree_Value = 0, Hashmap_Free_Value = 1 };

// Internal Prototypes - (Private to this Source File Only)
static int get_index(const char *key);
static long hash_code(const char *var);
static Carrier_t *carrier_create(const char *key, void *value);
static Carrier_t *hashmap_find(const char *key);
static int insert_at_index(int index, Carrier_t *entry);
static int hash_rehash(int new_capacity);
static int is_key(Carrier_t *entry, const char *key);
static void remove_entry(Carrier_t *entry, int dofree);
static int remove_key(const char *key, int dofree);
static void free_entry(Carrier_t *entry, int dofree);

/* Initialize a new HashMap
 * All Values stored must be dynamically allocated and are stored at void pointers.
 * - The function to properly free the object is required to be passed in. (free_value)
 * Rules for the HashMap
 * - Doubles when size > (initial_capacity>>1 + initial_capacity>>2)
 * - Halves when size < (initial_capcity>>2);
 * - Resets to initial_capacity on clear()
 * Returns: 0 on Success, ERROR Codes on Errors
 */
int hashmap_create(int initial_capacity, void (*free_value)(void *value)) {
  if(hashmap != NULL) {
    return Hashmap_Exists;
  }

  // Option to enter 0 (or < 0) to use a default size
  if(initial_capacity <= 0) {
    initial_capacity = HASHMAP_DEFAULT_INITIAL_SIZE;
  }
  
  // Verify there's a function to free the Value
  if(free_value == NULL) {
    return Hashmap_FreeValue_Missing;
  }

  // Allocate and Verify Memory for Symbol Table
  hashmap = malloc(sizeof(Hashmap_t));
  if(hashmap == NULL) {
    return Hashmap_Insufficient_Memory;
  }

  // Initialize the Memory for the Symbol Table
  // - This is a 1D array of Pointers to Carrier_t objects.
  hashmap->table = calloc(initial_capacity, sizeof(Carrier_t *));
  if(hashmap->table == NULL) {
    free(hashmap);
    return Hashmap_Insufficient_Memory;
  }

  hashmap->free_value = free_value;
  hashmap->size = 0; // Currently Used Indices
  hashmap->capacity = initial_capacity;  // Capacity of Indices for Use
  hashmap->base_capacity = initial_capacity;  // Base Capacity of Indices for Use (post Clear)
  return Hashmap_Success;
}

/* Checks if the Hashmap is Empty
 * Returns: 1 if Empty, 0 if Not-Empty, Hashmap_Uninitialized on Errors
 */
int hashmap_isEmpty() {
  return (hashmap != NULL)?hashmap->size==0:Hashmap_Uninitialized;
}

/* Gets the number of K,V entries in the HashMap
 * Returns: >0 for the Size, 0 if Empty, Hashmap_General_Error on Errors
 */
int hashmap_size() {
  return (hashmap != NULL)?hashmap->size:Hashmap_General_Error;
}

/* Gets the current number of Indices in the HashMap
 * Returns: >0 for the Capacity, 0 if None (Error Itself), Hashmap_General_Error on Errors
 */
int hashmap_capacity() {
  return (hashmap != NULL)?hashmap->capacity:Hashmap_General_Error;
}

/* Checks if the Hashmap contains a Key
 * Returns: 1 if Key Exists, 0 if No Such Key, ERROR on Errors
 */
int hashmap_containsKey(const char *key) {
  if(hashmap == NULL) {
    return Hashmap_Uninitialized;
  }
  if(key == NULL || strlen(key) == 0) {
    return Hashmap_Invalid_Key;
  }

  return hashmap_find(key)!=NULL;
}

/* Gets the value for a given key. Will not Free the Value Returned.
 * Returns: void *value if the key is found, NULL if No Such Key
 */
void *hashmap_get(const char *key) {
  if(hashmap == NULL || key == NULL) {
    return NULL;
  }

  Carrier_t *entry = hashmap_find(key);
  return (entry!=NULL)?entry->value:NULL;
}

/* Gets the value for a given key.
 * Returns: void *value if the key is found, NULL if No Such Key
 */
static Carrier_t *hashmap_find(const char *key) {
  if(hashmap == NULL || key == NULL) {
    return NULL;
  }

  int index = get_index(key);
  if(index == Hashmap_General_Error) {
    return NULL;
  }

  // Uses separate chaining, so no tombstones needed. 
  if(hashmap->table[index] == NULL) {
    return NULL;
  }

  /* Walk the linked list looking for a match variable name */
  Carrier_t *walker = hashmap->table[index];
  while(walker != NULL) {
    /* If a match is found, report it */
    if(is_key(walker, key)) {
      return walker;
    }
    walker = walker->next;
  }
  return NULL;
}

/* Adds a new Value to the Hashmap using the key.
 * Returns: Hashmap_Success or ERROR Code on Error.
 */
int hashmap_put(const char *key, void *value) {
  if(hashmap == NULL) {
    return Hashmap_Uninitialized;
  }
  if(key == NULL || strlen(key) == 0) {
    return Hashmap_Invalid_Key;
  }

  /* Try and update the existing valueue */
  Carrier_t *entry = hashmap_find(key);
  if(entry != NULL) {
    /* Check to see if this is a different value (update) */
    if(entry->value != value) {
      hashmap->free_value(entry->value);
      entry->value = value;
    }
    return Hashmap_Success;
  }

  /* Need to create a new entry */
  entry = carrier_create(key, value);
  if(entry == NULL) {
    return Hashmap_Insufficient_Memory;
  }

  /* Calculate the load and see if a rehash is needed before insert */
  /* - Doubles when new size > (initial_capacity>>1 + initial_capacity>>2) */
  /* - Special Case to handle int division, if new size is capacity (input on capacity = 1), then double */
  if(((hashmap->size + 1) >= ((hashmap->capacity>>1) + (hashmap->capacity>>2)) || 
      (hashmap->size + 1) >= hashmap->capacity)) {
    hash_rehash(hashmap->capacity * 2);
  }

  /* Get the hash code and then insert Symbol at the index */
  int index = get_index(entry->key);
  if(index == Hashmap_General_Error) {
    return Hashmap_Invalid_Key;
  }

  insert_at_index(index, entry);
  return Hashmap_Success;
}

/* Clears and Frees all Entries in the HashMap, Removes HashMap
 * Always Succeeds (no return)
 */
void hashmap_destroy() {
  if(hashmap == NULL) {
    return;
  }

  int ret = hashmap_clear();
  if(ret == Hashmap_Uninitialized) {
    return;
  }

  if(hashmap != NULL) {
    free(hashmap->table);
    free(hashmap);
    hashmap = NULL;
  }
}

/* Clears and Frees all Entries in the HashMap
 * Resets HashMap to initial capacity
 * Returns HashMap_Success or Error Condition
 */
int hashmap_clear() {
  if(hashmap == NULL) {
    return Hashmap_Uninitialized;
  }

  /* Iterate all Keys and Free Them */
  Carrier_t *walker = NULL;
  Carrier_t *reaper = NULL;
  int i = 0;
  for(i = 0; i < hashmap->capacity; i++) {
    walker = hashmap->table[i];
    while(walker != NULL) {
      reaper = walker;
      walker = walker->next;
      remove_entry(reaper, Hashmap_Free_Value);
    }
    hashmap->table[i] = NULL;
  }

  /* Reset to Base Hash Capacity */
  int ret = hash_rehash(hashmap->base_capacity);
  return ret;
}

/* Removes an Entry in the HashMap, will Rehash if needed after.
 * Will NOT Free the Value
 * Returns HashMap_Success or Error Condition
 */
int hashmap_remove(const char *key) {
  return remove_key(key, Hashmap_NoFree_Value);
}

/* Removes an Entry in the HashMap, will Rehash if needed after.
 * Will Free the Value
 * Returns HashMap_Success or Error Condition
 */
int hashmap_remove_free(const char *key) {
  return remove_key(key, Hashmap_Free_Value);
}

/* Removes an Entry in the HashMap, will Rehash if needed after.
 * Will Free the Value based on the dofree flag
 * Returns HashMap_Success or Error Condition
 */
static int remove_key(const char *key, int dofree) {
  if(hashmap == NULL) {
    return Hashmap_Uninitialized;
  }
  if(key == NULL || strlen(key) == 0) {
    return Hashmap_Invalid_Key;
  }

  /* If there's no such key, mission accomplished. */
  Carrier_t *entry = hashmap_find(key);
  if(entry == NULL) {
    return Hashmap_Success;
  } else {
    remove_entry(entry, dofree);
  }

  /* Calculate the load and see if a rehash is needed before insert */
  /* - Halves when size < (initial_capacity>>1) */
  if(hashmap->size < (hashmap->capacity>>1)) {
    hash_rehash(hashmap->capacity / 2);
  }

  return Hashmap_Success;
}

/* [Convenience Function] Prints all Keys (and their Indices)
 */
void hashmap_print_keys() {
  printf("Keys: \n");
  if(hashmap == NULL) {
    return;
  }

  /* Iterate all Indices and Keys, Printing Them */
  int i = 0;
  Carrier_t *walker = NULL;
  Carrier_t *end = NULL;
  for(i = 0; i < hashmap->capacity; i++) {
    if(hashmap->table[i] != NULL) {
      for(walker = hashmap->table[i]; walker != NULL; walker = walker->next) {
        end = walker;
        printf("[%2d]  %s\n", i, walker->key);
      }  
    }
  }
}

/* [Convenience Function] Prints a Description for the given Hashmap Error.
 */
void hashmap_print_error(int error) {
  if((error < HASHMAP_NUM_ERROR_CODES) && (error >= 0)) {
    printf("[HashMap Error %2d] %s\n", error, error_str[error]);
  }
  else {
    printf("[HashMap] No Such Error\n");
  }
}

/* Rehashes the HashMap to the new Capacity (Larger or Smaller)
 * Returns HashMap_Success or Error Condition
 */
static int hash_rehash(int new_capacity) {
  if(hashmap == NULL) {
    return Hashmap_Uninitialized;
  }
  if(new_capacity <= 0) {
    return Hashmap_Invalid_Capacity;
  }

  Carrier_t **new_table = calloc(new_capacity, sizeof(Carrier_t *));
  if(new_table == NULL) {
    printf("ERROR: Failed to Allocate Symtab Indices\n");
    return Hashmap_Insufficient_Memory;
  }

  /* Move the HashMap to the new Table, but hold the Old Table */
  Carrier_t **old_table = hashmap->table;
  hashmap->table = new_table;

  int old_capacity = hashmap->capacity;
  hashmap->capacity = new_capacity;
  hashmap->size = 0; // Will all be added back in properly below


  /* Iterate the old table and hash all the values into the new table.
   * Free the old symbols
   */
  Carrier_t *walker = NULL;
  Carrier_t *reaper = NULL;
  int i;
  for(i = 0; i < old_capacity; i++) {
    if(old_table[i] != NULL) {
      walker = old_table[i];
      while(walker != NULL) {
        reaper = walker;
        hashmap_put(walker->key, walker->value);
        walker = walker->next;
        // The object was moved to the next table, so cut the pointer here first.
        reaper->value = NULL;
        free_entry(reaper, Hashmap_Free_Value);
      }
    }
  }
  free(old_table);
  return Hashmap_Success;
}

/* Computes the Hash Code from a String
 * Returns the Hash Code
 */
static long hash_code(const char *var) {
  long code = 0;
  int i;
  int size = strlen(var);

  for(i = 0; i < size; i++) {
    code = (code + var[i]);
    if(size == 1 || i < (size - 1)) {
      code <<= 7;
    }
  }

  return code;
}

/* Creates a new Carrier entry object
 * Returns the Carrier or NULL on errors
 */
static Carrier_t *carrier_create(const char *key, void *value) {
  Carrier_t *entry = calloc(1, sizeof(Carrier_t));
  if(entry == NULL) {
    return NULL;
  }

  strncpy(entry->key, key, MAX_KEY_SIZE);
  entry->value = value;
  entry->next = NULL;
  entry->prev = NULL;
  return entry;
}

/* Inserts an Entry at the given Index
 * Returns Hashmap_Success or Error
 */
static int insert_at_index(int index, Carrier_t *entry) {
  if(hashmap == NULL) {
    printf("Error: Uninitialized Symbol Table\n");
    return Hashmap_Uninitialized;
  }
  if(index < 0 || index > hashmap->capacity) {
    return Hashmap_Invalid_Index;
  }

  /* Simple Case, nothing at index, so insert it */
  if(hashmap->table[index] == NULL) {
    hashmap->table[index] = entry;
  }
  /* Else, iterate to find the tail and insert there */
  else {
    Carrier_t *walker = hashmap->table[index];
    while(walker->next != NULL) {
      walker = walker->next;
    }
    walker->next = entry;
    entry->prev = walker;
  }
  hashmap->size++;
}

/* Gets the index for a Key
 * Returns the Index or Hashmap_General_Error (-1)
 */
static int get_index(const char *key) {
  if(hashmap == NULL || key == NULL || strlen(key) == 0) {
    return Hashmap_General_Error;
  }
  
  long code = hash_code(key);

  return code % hashmap->capacity;
}

/* Returns True if the Key matches the Entry's Key, otherwise False
 */
static int is_key(Carrier_t *entry, const char *key) {
  return !(strncmp(entry->key, key, MAX_KEY_SIZE));
}

/* Removes a Carrier Entry in the HashMap and Frees the Value if dofree is Hashmap_Free_Value */
static void remove_entry(Carrier_t *entry, int dofree) {
  if(entry == NULL) {
    return;
  }

  int index = get_index(entry->key);

  // Case where this is the first item in the Index, simply update the table around it.
  if(entry->prev == NULL) {
    hashmap->table[index] = entry->next;
  } 
  // Otherwise, bridge around it forward.
  else {
    entry->prev->next = entry->next;
  }
  // Either way, bridge around it backwards
  if(entry->next) {
    entry->next->prev = entry->prev;
  }
  // Now, free the entry
  entry->next = NULL;
  free_entry(entry, dofree);
  // And adjust the hashmap size
  hashmap->size--;
}

/* Free the memory for an Entry and will call the free_value function if dofree is Hashmap_Free_Value. */
static void free_entry(Carrier_t *entry, int dofree) {
  if(hashmap != NULL && entry != NULL) {
    if(dofree == Hashmap_Free_Value) {
      hashmap->free_value(entry->value);
    }
  }
  free(entry);
}
