## my $namespace = 'rbhash';
## my $min_bits  = 8;
## my $max_bits  = 8;
## my @default_compare_args= ('void *userdata', 'int (*cmp_cb)(void *, size_t)');
## my $default_compare_fn = sub($node_id){ "cmp_cb(userdata, $node_id)" };
## my @treeprint_args;
## my $debug= 0;

## my $NAMESPACE= uc($namespace);

## my @bits= map +(1<<$_), (log($min_bits)/log(2)) .. (log($max_bits)/log(2));

##sub word_type($bits) { 'uint'.$bits.'_t' }

/* MAX_TREE_HEIGHT is the maximum number of nodes from root to leaf in any
 * correctly balanced tree.
 */
## #scope PUBLIC;
## for my $bits (@bits) {
#define ${NAMESPACE}_MAX_TREE_HEIGHT_$bits  ${{2*($bits-1)+1}}
#define ${NAMESPACE}_MAX_ELEMENTS_$bits     0x${{sprintf "%X", 2**($bits-1)-2}}
## }

/* This macro selects the number of hash buckets to allocate, given the max
 * number of elements.  This can be increased for better hashing,
 * or decreased for memory savings.
 */
#ifndef ${NAMESPACE}_TABLE_BUCKETS
#define ${NAMESPACE}_TABLE_BUCKETS(capacity) (capacity)
#endif
/* This macro tells you the array index (starting from rbtable)
 * of the hash bucket for 'hash_code'.
 */
#define ${NAMESPACE}_TABLE_WORD_OFS(capacity) ( (capacity)*2 + 3 )
#define ${NAMESPACE}_TABLE_WORD_IDX(capacity, hash_code) \
         ((hash_code) % ${NAMESPACE}_TABLE_BUCKETS(capacity))
/* This macro tells you the array index (starting from rbtable)
 * of the slot used as the "parent pointer" for the root of the tree.
 * There are many trees, so this slot is used temporarily by each
 * algorithm that needs a root pointer.
 */
#define ${NAMESPACE}_TREE_TMPROOT_IDX(capacity) ( (capacity)*2+2 )

/* This macro selects the word size needed to index 'capacity' number of
 * user elements.
 */
#define ${NAMESPACE}_SIZEOF_WORD(capacity) ( \
## for my $bits (reverse @bits) {
##   if ($bits > $min_bits) {
           (capacity) > ${NAMESPACE}_MAX_ELEMENTS_${{$bits/2}}? ${{$bits/8}} : \
##   } else {
           ${{ $bits/8 }} \
##   }
## }
        )

/* This macro defines the total size (in bytes) of the rbhash array
 * for a given number of allocated elements.  This does not include the
 * elements, since those are whatever size the user wants them to be.
 */
#define ${NAMESPACE}_SIZEOF(capacity) ( \
           ${NAMESPACE}_SIZEOF_WORD(capacity) * ( \
             (capacity)*2 + 3 \
             + ${NAMESPACE}_TABLE_BUCKETS(capacity) \
           ) \
        )

/* This macro defines the maximum required size of a tree_path structure
 * for a given capacity, in bytes.  Note that these bytes need aligned to
 * at least the alignment of a pointer.
 */
#define ${NAMESPACE}_SIZEOF_PATH(capacity) ( \
           sizeof(struct ${namespace}_tree_path) \
           + ${NAMESPACE}_SIZEOF_WORD(capacity) * 16 \
        )
#define ${NAMESPACE}_DECLARE_STACK_PATH(varname, capacity) \
   struct ${namespace}_tree_path *varname= \
     (struct ${namespace}_tree_path *) alloca( \
         ${NAMESPACE}_SIZEOF_PATH(capacity) \
     );
#define ${NAMESPACE}_INIT_STACK_PATH(path, size) \
   do { \
      (path)->len= 0; \
      (path)->lim= ((size) - (sizeof(struct ${namespace}_tree_path)) / ${NAMESPACE}_SIZEOF_WORD(capacity)); \
      (path)->u$min_bits.bucket= 0; \
   } while(0)

//include "rbhash.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

struct ${namespace}_tree_path {
   int len, lim;
   union {
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
      struct {
         $word_t *bucket;
         $word_t refs[];
      } u$bits;
## }
   };
};
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
extern size_t ${namespace}_tree_insert_$bits($word_t *rbhash, size_t capacity, struct ${namespace}_tree_path *path, size_t node);
extern size_t ${namespace}_tree_delete_$bits($word_t *rbhash, size_t capacity, struct ${namespace}_tree_path *path);
## }

extern size_t ${namespace}_tree_insert(void *rbhash, size_t capacity, struct ${namespace}_tree_path *path, size_t node) {
##   for my $bits (@bits) {
##     my $word_t= word_type($bits);
   if (capacity <= ${NAMESPACE}_MAX_ELEMENTS_$bits)
      ${namespace}_tree_insert_$bits(($word_t*) rbhash, capacity, path, node);
##   }
   return 0;
}

extern size_t ${namespace}_tree_delete(void *rbhash, size_t capacity, struct ${namespace}_tree_path *path) {
##   for my $bits (@bits) {
##     my $word_t= word_type($bits);
   if (capacity <= ${NAMESPACE}_MAX_ELEMENTS_$bits)
      ${namespace}_tree_delete_$bits(($word_t*) rbhash, capacity, path);
##   }
   return 0;
}

/* Find a node in the hash table, or tree.  Returns the node_id, or 0 if no
 * nodes match.
 *
 * This is a simplified version of find_path that doesn't keep track of the
 * path through the tree, saving time but not facilitating inserts or deletes.
 */
size_t ${namespace}_find(
   void *rbhash, size_t capacity,
   size_t hash_code, @default_compare_args
) {
   size_t node;
   int cmp;
   if (!${NAMESPACE}_TABLE_BUCKETS(capacity)) return 0;
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
##   my $else= $bits > $min_bits? ' else':'';
  $else if (capacity <= ${NAMESPACE}_MAX_ELEMENTS_$bits) {
      node= (($word_t *)rbhash)[ ${NAMESPACE}_TABLE_WORD_IDX(capacity, hash_code) ];
      while (node && (cmp= ${{$default_compare_fn->('node')}}))
         node= (($word_t *)rbhash)[ (node<<1) | (cmp < 0? 0 : 1) ] >> 1;
   }
## }
   return node;
}

/* Find a node in the hash table, and record the path to arrive at the node
 * or the node pointer where it would exist.  The path can be used for
 * inserting or deleting without re-comparing any elements.
 *
 * See documentation of ${NAMESPACE}_SIZEOF_PATH,
 * ${NAMESPACE}_DECLARE_STACK_PATH, and ${NAMESPACE}_INIT_STACK_PATH
 * for details on how to create the 'path' parameter.
 */
size_t ${namespace}_find_path(
   void *rbhash, size_t capacity,
   struct ${namespace}_tree_path *path, size_t sizeof_path,
   size_t hash_code, @default_compare_args
) {
   size_t ref, node= 0;
   int cmp, p_i= 0, p_lim;
   if (!${NAMESPACE}_TABLE_BUCKETS(capacity)) { path->len= -1; return 0; }
   ${NAMESPACE}_INIT_STACK_PATH(path, sizeof_path);
   p_lim= path->lim;
   if (p_lim < 1) { path->len= -1; return 0; }
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
##   my $else= $bits > $min_bits? ' else':'';
  $else if (capacity <= ${NAMESPACE}_MAX_ELEMENTS_$bits) {
      $word_t *rbhash_w= ($word_t*) rbhash;
      path->u$bits.refs[0]= 0;
      node= *(
         path->u$bits.bucket= rbhash_w + ${NAMESPACE}_TABLE_WORD_OFS(capacity)
            + ${NAMESPACE}_TABLE_WORD_IDX(capacity, hash_code)
      );
      while (node && (cmp= ${{$default_compare_fn->('node')}})) {
         ref= (node<<1) | (cmp < 0? 0 : 1);
         if (++p_i >= p_lim) { path->len= -1; return 0; }
         path->u$bits.refs[p_i]= ref;
         node= rbhash_w[ref] >> 1;
      }
   }
## }
   else p_i= -1;
   path->len= p_i;
   return node;
}

/* Insert a node into the hashtable, storing collisions in a tree.
 * If it finds a node with same key, it returns that index and does not insert
 * the new node, else it will insert and return your 'new_node' value.
 * If it returns node 0, you have a corrupted data structure.
 */
extern size_t ${namespace}_insert(
   void *rbhash, size_t capacity,
   size_t new_node, size_t hash_code, @default_compare_args
) {
   size_t ref, node= 0;
   int cmp, p_i= 0, p_lim;
   ${NAMESPACE}_DECLARE_STACK_PATH(path, capacity)
   if (!${NAMESPACE}_TABLE_BUCKETS(capacity)) return 0;
   ${NAMESPACE}_INIT_STACK_PATH(path, ${NAMESPACE}_SIZEOF_PATH(capacity));
   p_lim= path->lim;
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
##   my $else= $bits > $min_bits? ' else':'';
  $else if (capacity <= ${NAMESPACE}_MAX_ELEMENTS_$bits) {
      $word_t *rbhash_w= ($word_t*) rbhash,
         *bucket= rbhash_w + ${NAMESPACE}_TABLE_WORD_OFS(capacity)
            + ${NAMESPACE}_TABLE_WORD_IDX(capacity, hash_code);
      if (!(node= *bucket)) {
         *bucket= new_node;
         return new_node;
      }
      else {
         do {
            if (!(cmp= ${{$default_compare_fn->('node')}}))
               return node;
            ref= (node<<1) | (cmp < 0? 0 : 1);
            if (++p_i >= p_lim)
               return 0;
            path->u$bits.refs[p_i]= ref;
            node= rbhash_w[ref] >> 1;
         } while (node);
         // Handle simple case of adding to black parent without invoking balance.
         if (p_i == 1 || !(rbhash_w[path->u$bits.refs[p_i-1]] & 1)) {
            rbhash_w[ref]= (new_node << 1) | 1;
            return new_node;
         }
         path->u$bits.bucket= bucket;
         path->len= p_i+1;
         return ${namespace}_tree_insert_$bits(rbhash_w, capacity, path, new_node);
      }
   }
## }
   return 0;
}

/* Find and delete a node in the hashtable.  If found, this returns the node_id
 * that was removed.  If not found (or if the data structure is currupt) this
 * returns 0.
 */
extern size_t ${namespace}_delete(
   void *rbhash, size_t capacity,
   size_t hash_code, @default_compare_args
) {
   size_t ref= 0, node= 0;
   int cmp, p_i= 0, p_lim;
   ${NAMESPACE}_DECLARE_STACK_PATH(path, capacity)
   if (!${NAMESPACE}_TABLE_BUCKETS(capacity)) return 0;
   ${NAMESPACE}_INIT_STACK_PATH(path, ${NAMESPACE}_SIZEOF_PATH(capacity));
   p_lim= path->lim;
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
##   my $else= $bits > $min_bits? ' else':'';
  $else if (capacity <= ${NAMESPACE}_MAX_ELEMENTS_$bits) {
      $word_t *rbhash_w= ($word_t*) rbhash, cur, ch_ref1, ch_ref2,
         *bucket= rbhash_w + ${NAMESPACE}_TABLE_WORD_OFS(capacity)
            + ${NAMESPACE}_TABLE_WORD_IDX(capacity, hash_code);
      if ((cur= *bucket << 1)) {
         while ((cmp= ${{$default_compare_fn->('cur >> 1')}})) {
            ref= (cur|1) ^ (cmp < 0? 1 : 0);
            cur= rbhash_w[ref];
            if (cur <= 1 || ++p_i >= p_lim)
               return 0;
            path->u$bits.refs[p_i]= ref;
         }
         // Directly handle the easiest cases, for speed.
         // Is this node a leaf or almost a leaf?
         ch_ref1= rbhash_w[cur], ch_ref2= rbhash_w[cur^1];
         rbhash_w[cur]= 0;   // reset node to zeroes
         rbhash_w[cur^1]= 0; //
         if (!ch_ref1 || !ch_ref2) {
            if (!p_i) { // at the root, need to modify *bucket
               // Replace node with child node.
               // Also handles the case that both were 0.
               *bucket= (ch_ref1|ch_ref2) >> 1;
               return cur >> 1;
            }
            else if ((cur&1) | ch_ref1 | ch_ref2) { // red leaf or black with red child
               rbhash_w[ref]= (ch_ref1|ch_ref2) >> 1 << 1;
               return cur >> 1;
            }
            // else black leaf. That's hard.
         }
         path->u$bits.bucket= bucket;
         path->len= p_i+1;
         return ${namespace}_tree_delete_$bits(rbhash_w, capacity, path);
      }
      else return 0;
   }
## }
   return 0;
}

/* The balance function takes a pointer to the data structure and a pointer
 * to the *end* of an array of word indices that describe the path from root
 * to the current node.  This array *must* begin with the number 0.
 * The end element of parent_refs should be the index of the int referring to
 * the parent node of the node that was just added.  (Balance is never called
 * for the root of the tree or either of its immediate children, which are
 * always balanced when first added)
 *
 * There is one balance function for each bit width.  The balance function
 * could be wrapped with a bit-width-selector, but the code that calls it
 * already knows the bit-width.
 * See documentation of ${NAMESPACE}_SIZEOF_PATH,
 * ${NAMESPACE}_DECLARE_STACK_PATH, and ${NAMESPACE}_INIT_STACK_PATH
 * for details on how to create the 'path' parameter.
 */
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
extern size_t ${namespace}_tree_insert_$bits(
   $word_t *rbhash, size_t capacity,
   struct ${namespace}_tree_path *path,
   size_t new_node
) {
   // For this entire function, node values are shifted left and the low bit is
   // either the color (on node values) or the left/right offset (on ref values)
   // See notes on 'delete' function.
   $word_t root_ref= ${NAMESPACE}_TREE_TMPROOT_IDX(capacity);
   int p_i;
   // Any path shorter than 2 means the node becomes the new tree root
   if (path->len < 2) {
      assert( ! *(path->u$bits.bucket) );
      *(path->u$bits.bucket)= new_node;
      return new_node;
   }
   // Use the temporary root-ref slot of the rbhash to point to the root node.
   // This saves 'if' statements during rotations.
   path->u$bits.refs[0]= root_ref;
   rbhash[root_ref]= *path->u$bits.bucket << 1;
   // add new_node to the final parent-ref of the path
   p_i= path->len-1;
   assert( !( rbhash[path->u$bits.refs[p_i]] ) );
   rbhash[path->u$bits.refs[p_i--]]= (new_node << 1) | 1; // and make it red
   // 'pos' will be the parent node of that.
   while (p_i > 0) {
      $word_t pos_ref= path->u$bits.refs[p_i--]; // p_i could reach -1 already at this point
      $word_t pos= rbhash[pos_ref];
      $word_t parent_ref= path->u$bits.refs[p_i];
      // if current is a black node, no rotations needed
      if (!(pos & 1))
         break;
      // pos is red, its new child is red, and parent will be black.
      // if the sibling is also red, we can pull down the color black from the parent
      if (rbhash[pos_ref^1]&1) {
         rbhash[pos_ref] ^= 1;       // toggle color of pos
         rbhash[pos_ref^1] ^= 1;     // toggle color of sibling
         rbhash[parent_ref] ^= 1;    // toggle color of parent
         // Now pos is black.
         // Jump twice up the tree so that once again, pos has one red child.
         p_i--;
      }
      else {
         // Need a rotation.
         // sibling is black.
         // if the imbalanced child (red node) is on the same side as the parent,
         //  need to rotate those lower nodes to the opposite side in preparation
         //  for the rotation.
         // e.g. if pos_ref is leftward (even) and pos's rightward child (odd) is the red one...
         $word_t child_ref= (pos|1) ^ (pos_ref&1);
         $word_t child= rbhash[child_ref];
         if (child&1) {
            // rotate pos toward [side] so parent's [side] now points to pos's [otherside]
            // set pos's child-ref to child's [otherside] ref
            $word_t grandchild_ref= (child|1) ^ (child_ref&1);
            rbhash[child_ref]= rbhash[grandchild_ref];
            // set child's [side] to pos
            rbhash[grandchild_ref]= pos;
            pos= child|1; // keep pos as a red node, soon to become black
            // parent's [side] has not been updated here, but is about to become 'child'
            child= rbhash[grandchild_ref^1];
         }
         // Now we can rotate toward parent to balance the tree.
         rbhash[pos_ref]= child;
         rbhash[child_ref]= pos_ref|1; // = parent, colored red.  simplification of ((pos_ref>>1)<<1)|1
         rbhash[parent_ref]= pos^1; // also make pos black
         // rotation finished, exit.
         break;
      }
   }
   // If rotated the root, store the root back into the hash bucket
   *path->u$bits.bucket= rbhash[root_ref] >> 1;
   return new_node;
}
## }

/* Prune a black leaf (node with no children) from the tree.
 *
 * This is the interesting half of the Red/Black deletion algorithm.
 * The parameters are the rbhash array, and an array of refs of each
 * of the parents' pointers to the next node.  The parent_refs pointer is to
 * the *last element* of that array, which is a ref to the node to be deleted.
 * The parent_refs array must be terminated with an initial 0 value.
 *
 * For this entire function, node values are shifted left and the low bit is
 * either the color, or the left/right offset.
 * Spelled out:
 *   - pos_ref
 *       The index within rbhash of the integer that refers to pos
 *   - pos_ref>>1
 *       The parent node_id of pos.
 *   - pos_ref&1
 *       True if the parent's right subtree points to pos
 *   - pos_ref^1
 *       The index within rbhash of the integer that refers to pos's sibling
 *   - rbhash[pos_ref] >> 1
 *       The actual node_id of pos.
 *   - pos = rbhash[pos_ref]
 *       The index of the integer referring to one of pos's subtrees
 *   - pos&1:
 *       Whether pos is a red node (assuming pos was loaded from pos_ref)
 *   - pos^1:
 *       The index of the integer referring to the other sub-tree of pos.
 *   - pos|1:
 *       The index of the integer referring to the right subtree of pos
 *   - pos >> 1 << 1:
 *       The index of the integer referring to the left subtree of pos
 */
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
extern size_t ${namespace}_tree_delete_$bits(
   $word_t *rbhash, size_t capacity,
   struct ${namespace}_tree_path *path
) {
   $word_t root_ref= ${NAMESPACE}_TREE_TMPROOT_IDX(capacity);
   $word_t pos, pos_ref, *parent_refs= path->u$bits.refs, ch1, ch2, sibling;
   int p_i= path->len-1, p_lim= path->lim;
   // Path should be at least 1 element (which would mean to delete the root)
   if (path->len < 1)
      return 0;
   // Use the temporary root-ref slot of the rbhash to point to the root node.
   // This saves 'if' statements later.
   parent_refs[0]= root_ref;
   rbhash[root_ref]= *path->u$bits.bucket << 1;
   // Now read the final ref to find 'pos_ref' and 'pos'
   pos_ref= parent_refs[p_i];
   pos= rbhash[pos_ref];
   // If pos has children, find a leaf to swap with.
   // Then delete this node in the leaf's position.
   ch1= rbhash[pos], ch2= rbhash[pos^1];
   if (ch1 || ch2) {
      if (ch1 && ch2) {
         int orig_p_i= p_i;
         $word_t tmp, alt= pos, alt2, ref;
         // descend one level to the left
         if (++p_i >= p_lim) return 0;
         parent_refs[p_i]= ref= (pos >> 1 << 1); // go left;
         // descend as many levels as possible to the right
         while ((alt= rbhash[ref= alt|1])) {
            if (++p_i >= p_lim) return 0;
            parent_refs[p_i]= ref;
         }
         // 'alt' is the node we swap with.
         alt= rbhash[parent_refs[p_i]];
         // is there one to the left?
         if ((alt2= rbhash[alt])) {
            assert(alt2&1);
            // it is required to be a red leaf, so replace alt with it
            rbhash[parent_refs[p_i]]= alt2^1;
            rbhash[alt2]= 0;
            rbhash[alt2^1]= 0;
            // Now substitute this for pos and we're done.
            rbhash[alt >> 1 << 1]= rbhash[pos >> 1 << 1];
            rbhash[alt|1]= rbhash[pos|1];
            rbhash[pos_ref]= (alt >> 1 << 1) | (pos&1); // preserve color of pos
         }
         else {
            // swap colors of alt and pos
            alt ^= pos & 1;
            pos ^= alt & 1;
            alt ^= pos & 1;
            rbhash[alt|1]= rbhash[pos|1];         // copy right
            rbhash[(alt|1)^1]= rbhash[(pos|1)^1]; // copy left
            rbhash[pos_ref]= alt;
            if (pos & 1)
               // read leaf, remove it
               rbhash[parent_refs[p_i]]= 0;
            else {
               // black leaf, need balancing
               pos_ref= parent_refs[p_i];
               rbhash[pos_ref]= pos;
               rbhash[pos]= 0;
               rbhash[pos^1]= 0;
               // pos and alt are in reverse sort-order, but we're deleting pos now.
               goto del_black_leaf; // https://xkcd.com/292/
            }
         }  
      }
      else {
         // Node is black with one child.  Swap with it.
         rbhash[pos_ref]= (ch1 | ch2) >> 1 << 1; // and make it black
      }
   }
   // Now, pos is a leaf node.  If it is red, just remove it.
   else if (pos&1)
      rbhash[pos_ref]= 0;
   // Black node with no children.  Now it gets interesting.
   else {
      del_black_leaf:
      // The tree must have the same number of black nodes along any path from root
      // to leaf.  We want to remove a black node, disrupting the number of black
      // nodes along the path from the root to the current leaf.  To correct this,
      // we must either reduce all other paths, or add a black node to the current
      // path.

      // Loop until the current node is red, or until we get to the root node.
      sibling= rbhash[pos_ref^1];
      do {
         $word_t near_nephew_ref, near_nephew;
         // If the sibling is red, we are unable to reduce the number of black
         //  nodes in the sibling tree, and we can't increase the number of black
         //  nodes in our tree..  Thus we must do a rotation from the sibling
         //  tree to our tree to give us some extra (red) nodes to play with.
         // This is Case 1 from the text
         if (sibling&1) {
            // node is black and sibling is red
            // get ref to sibling's near subtree
            near_nephew_ref= (sibling ^ 1) | (pos_ref&1);
            // sibling is new parent, and now black.
            if (p_i)
               rbhash[parent_refs[p_i]]= sibling ^ 1;
            // move sibling's child under parent, becoming new sibling (which is black)
            sibling= rbhash[near_nephew_ref];
            rbhash[pos_ref^1]= sibling;
            rbhash[near_nephew_ref]= pos_ref|1; // former sibling sameside tree = parent, now red
            if (++p_i >= p_lim)
               return 0;
            parent_refs[p_i] = near_nephew_ref; // insert new parent into list
         }
         // sibling will be black here

         // If the sibling is black and both children are black, we have to
         //  reduce the black node count in the sibling's tree to match ours.
         // This is Case 2a from the text.
         near_nephew_ref= sibling | (pos_ref&1);
         near_nephew= rbhash[near_nephew_ref];
         if (!((near_nephew|rbhash[near_nephew_ref^1]) & 1)) {
            assert(sibling > 1);
            rbhash[pos_ref^1] |= 1; // change sibling to red
            // Now we move one level up the tree to continue fixing the
            // other branches.
            if (p_i < 1) break;
            pos_ref= parent_refs[p_i--];
            if (rbhash[pos_ref]&1) {
               // Now, make the current node black (to fulfill Case 2b)
               rbhash[pos_ref] ^= 1;
               break;
            }
            sibling= rbhash[pos_ref^1];
         }
         else {
            // sibling will be black with 1 or 2 red children here

            // If one of the sibling's children are red, we again can't make the
            //  sibling red to balance the tree at the parent, so we have to do a
            //  rotation.  If the "near" nephew is red and the "far" nephew is
            //  black, we need to rotate that tree away before rotating the
            //  parent toward.
            // After doing a rotation and rearranging a few colors, the effect is
            //  that we maintain the same number of black nodes per path on the far
            //  side of the parent, and we gain a black node on the current side,
            //  so we are done.
            if (near_nephew&1) {
               // Case 3 from the text, double rotation
               $word_t tmp_ref= near_nephew ^ (pos_ref&1); // near nephew's far child
               rbhash[near_nephew_ref]= rbhash[tmp_ref];
               rbhash[pos_ref^1]= near_nephew;
               rbhash[tmp_ref]= sibling;
               sibling= near_nephew ^ 1; // make it black
               near_nephew_ref= sibling | (pos_ref&1);
            }
            else
               rbhash[near_nephew_ref^1] ^= 1; // far nephew becomes black
            // now Case 4 from the text
            assert(sibling > 1);
            rbhash[pos_ref^1]= rbhash[near_nephew_ref];
            // parent becomes black, balancing current path
            rbhash[near_nephew_ref]= pos_ref >> 1 << 1; 
            // Sibling assumes parent's color and position
            rbhash[parent_refs[p_i]]= sibling | (rbhash[parent_refs[p_i]]&1);
            break;
         }
      } while (pos_ref > 1);
   }
   // in case root_ref changed
   *path->u$bits.bucket= rbhash[root_ref] >> 1;
   // clean the 'pos' node for future use
   rbhash[pos]= 0;
   rbhash[pos^1]= 0;
   return pos >> 1;
}
## }


#include <stdio.h>
#include <string.h>
#include <stdbool.h>

## for my $bits (@bits) {
##   my $word_t= word_type($bits);
##   my $max_tree_height= "${NAMESPACE}_MAX_TREE_HEIGHT_$bits";
// Handy for gdb: "p ${namespace}_treeprint_$bits(rbhash, capacity, i, i, stdout)"
static size_t ${namespace}_treeprint_$bits(
   $word_t *rbhash, $word_t max_node, $word_t node, $word_t mark_node,
   void* userdata, void (*print_node)(void*,size_t,FILE*), FILE * out
) {
   $word_t node_path[ 1+$max_tree_height ];
   bool cycle;
   int i, pos, step= 0;
   size_t nodecount= 0;
   if (!node) {
      fputs("(empty tree)\n", out);
      return 0;
   }
   node_path[0]= 0;
   node_path[pos= 1]= node << 1;
   while (node && pos) {
      switch (step) {
      case 0:
         // Check for cycles
         cycle= false;
         for (i= 1; i < pos; i++)
            if ((node_path[i]>>1) == (node_path[pos]>>1))
               cycle= true;
         
         // Proceed down right subtree if possible
         if (!cycle && pos < $max_tree_height && node <= max_node && rbhash[(node<<1)|1]) {
            node= rbhash[(node<<1)|1] >> 1;
            node_path[++pos]= node << 1;
            continue;
         }
      case 1:
         // Print tree branches for nodes up until this one
         for (i= 2; i < pos; i++)
            fputs((node_path[i]&1) == (node_path[i+1]&1)? "    " : "   |", out);
         if (pos > 1)
            fputs((node_path[pos]&1)? "   `" : "   ,", out);
         
         // Print content of this node
         fprintf(out, "--%c%c%c #%ld%s ",
            (node == mark_node? '(' : '-'),
            (node > max_node? '!' : (rbhash[ (node_path[pos-1]|1) ^ (node_path[pos]&1) ]&1)? 'R':'B'),
            (node == mark_node? ')' : ' '),
            (long) node,
            cycle? " CYCLE DETECTED"
               : pos >= $max_tree_height? " MAX DEPTH EXCEEDED"
               : node > max_node? " VALUE OUT OF BOUNDS"
               : ""
         );
         if (print_node) print_node(userdata, node, out);
         fputs("\n", out);
         ++nodecount;
         
         // Proceed down left subtree if possible
         if (!cycle && pos < $max_tree_height && node <= max_node && rbhash[node<<1]) {
            node= rbhash[node<<1] >> 1;
            node_path[++pos]= (node << 1) | 1;
            step= 0;
            continue;
         }
      case 2:
         // Return to parent
         step= (node_path[pos]&1) + 1;
         node= node_path[--pos] >> 1;
         cycle= false;
      }
   }
   return nodecount;
}
## }

void ${namespace}_print(
   void *rbhash, size_t capacity, void* userdata,
   void (*print_node)(void*,size_t,FILE*), FILE *out
) {
   size_t n_buckets= ${NAMESPACE}_TABLE_BUCKETS(capacity), node, used= 0, collision= 0;
   fprintf(out, "# rbhash for %ld elements, %ld hash buckets\n", (long) capacity, (long) n_buckets);
## for my $bits (@bits) {
##   my $word_t= word_type($bits);
##   my $else= $bits > $min_bits? ' else':'';
  $else if (capacity <= ${NAMESPACE}_MAX_ELEMENTS_$bits) {
      $word_t *nodes= ($word_t*) rbhash;
      $word_t *table= nodes + ${NAMESPACE}_TABLE_WORD_OFS(capacity);
      int i;
      for (i= 0; i < n_buckets; i++) {
         if (i && (i & 0xF) == 0)
            fprintf(out, "# bucket 0x%lx\n", i);
         if (table[i]) {
            ++used;
            collision += ${namespace}_treeprint_$bits(rbhash, capacity, table[i], 0, userdata, print_node, out) - 1;
         } else
            fprintf(out, "-\n");
      }
   }
## }
   fprintf(out, "# used %ld / %ld buckets, %ld collisions\n", (long) used, (long) n_buckets, (long) collision);
}

struct cmp_args {
   int *el;
   int hash_code;
   int key;
};

int cmp_el(void *datap, size_t node) {
   struct cmp_args *data= (struct cmp_args *) datap;
   return data->key < data->el[node]? -1 : data->key > data->el[node]? 1 : 0;
}
void print_node(void *datap, size_t node, FILE *out) {
   struct cmp_args *data= (struct cmp_args *) datap;
   fprintf(out, "%ld", (long) data->el[node]);
}

#define N 16
int main() {
   int el[N];
   struct cmp_args cmp_args= { el, 0, 0 };
   uint8_t words[RBHASH_SIZEOF(N)];
   memset(el, 0, sizeof(el));
   memset(words, 0, sizeof(words));
   el[0]= 0;
   while (el[0] < N) {
      if (!scanf("%d", &cmp_args.key)) return 0;
      if (cmp_args.key < 0) {
         int x= rbhash_delete(words, N, 0, &cmp_args, cmp_el);
         if (x) printf("Deleted node %ld\n", (long)x);
         else printf("Not found, or err\n");
      }
      else if (cmp_args.key > 0) {
         if (el[0] >= N-1)
            printf("Array full\n");
         else {
            el[++el[0]]= cmp_args.key;
            int x= rbhash_insert(words, N, el[0], 0, &cmp_args, cmp_el);
            if (x != el[0])
               printf("insert failed: %ld\n", (long) x);
            else
               printf("inserted as node %ld\n", (long) x);
         }
      }
      rbhash_print(words, N, &cmp_args, print_node, stdout);
   }
}