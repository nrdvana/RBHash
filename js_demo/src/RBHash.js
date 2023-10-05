/* RBHash class, derived from rbhash.c in this same repository.
 * Copyright 2023, Michael Conrad
 */
export default class RBHash {
   constructor(capacity, n_buckets) {
      this.capacity= capacity;
      this.n_buckets= n_buckets > 1? n_buckets : 1;
      this.table_ofs= (capacity+1)*2
      this.array= new Array(this.table_ofs + n_buckets);
      this.array.fill(0);
   }
   batch_insert(n_values, hash_fn, cmp_fn) {
      for (let i= 1; i <= n_values; i++) {
         let hashcode= hash_fn(i)
         if (hashcode != null)
            this.insert(i, hash_fn(i), (i2) => cmp_fn(i, i2), null)
      }
   }
   async insert(node_id, hash_code, cmp_fn, showstate) {
      let rbhash= this.array;
      let bucket= hash_code % this.n_buckets;
      let bucket_ofs= this.table_ofs + bucket;
      let state= {
         message: ["Hash code "+hash_code+" mod "+this.n_buckets+" = "+bucket],
         highlight: { [bucket_ofs]: 'read' },
         newnode_at: bucket_ofs
      };
      if (!rbhash[bucket_ofs]) {
         if (showstate) await showstate(state)
         rbhash[bucket_ofs]= node_id << 1;
         state.message.push("Node becomes root of tree");
         state.finished= true;
         state.highlight[bucket_ofs]= 'write';
         if (showstate) await showstate(state)
      }
      else {
         let at_node= rbhash[bucket_ofs] >> 1
         let path= [ bucket_ofs ]
         let cmp;
         state.message.push('Bucket is full...')
         do {
            state.message.push("Compare to node "+at_node);
            state.insert_at= [ node_id, at_node ];
            if (showstate) await showstate(state)
            if (!(cmp= cmp_fn(at_node))) {
               state.message.push("Found a duplicate, will not insert");
               state.finished= true;
               if (showstate) await showstate(state)
               return at_node;
            }
            let ref= (at_node<<1) | (cmp < 0? 0 : 1);
            path.push(ref);
            if (path.length > 128) throw "BUG";
            at_node= rbhash[ref] >> 1;
         } while (at_node);
         let pos_ref= path.pop();

         rbhash[pos_ref]= (node_id << 1) | 1;
         state.message.push('Insert '+node_id+' under '+(pos_ref>>1))
         state.message.push('Leaf nodes are Red')
         state.highlight= { ['node'+(pos_ref>>1)]: 1, ['node'+node_id]: 2 }
         if (showstate) await showstate(state);
         
         while (path.length > 1) {
            let pos_ref= path.pop();
            let pos= rbhash[pos_ref];
            let parent_ref= path[path.length-1];
            state.message.push('Balance at node '+(pos>>1));
            state.highlight= { ['node'+(pos>>1)]: 2 }
            state.cur_node= pos_ref>>1;
            // if current is a black node, no rotations needed
            if (!(pos & 1)) {
               state.message.push('Node is Black, no rotations needed.')
               break
            }
            // pos is red, its new child is red, and parent will be black.
            // if the sibling is also red, we can pull down the color black from the parent
            // if not, need a rotation.
            state.highlight['node'+(rbhash[pos_ref^1]>>1)]= 1
            if (!(rbhash[pos_ref^1] & 1)) {
               // Sibling is black, need a rotation.
               state.message.push('Node is Red with Black sibling. Need a rotation.')
               // if the imbalanced child (red node) is on the same side as the parent,
               //  need to rotate those lower nodes to the opposite side in preparation
               //  for the rotation.
               // e.g. if pos_ref is leftward (even) and pos's rightward child (odd) is the red one...
               let child_ref= pos ^ (pos_ref&1);
               let child= rbhash[child_ref];
               state.highlight['node'+(child>>1)]= 'read'
               if (child&1) {
                  state.message.push('Red Child is same side as Parent, so need to rotate away from Parent');
                  if (showstate) await showstate(state);

                  // rotate pos toward [side] so parent's [side] now points to pos's [otherside]
                  // set pos's child-ref to child's [otherside] ref
                  let near_grandchild_ref= child ^ (child_ref&1);
                  rbhash[child_ref]= rbhash[near_grandchild_ref];
                  // set child's [side] to pos
                  rbhash[near_grandchild_ref]= pos;
                  pos= child; // keep pos as a red node, soon to become black
                  rbhash[pos_ref]= child;
                  // parent's [side] has not been updated here, but is about to become 'child'
                  child_ref= near_grandchild_ref^1;
                  child= rbhash[child_ref];

                  state.highlight= { ['node'+(pos>>1)]:2, ['node'+(child>>1)]:1 }
               }
               // Now we can rotate toward parent to balance the tree.
               state.message.push("Rotate Red node up to Parent's position");
               if (showstate) await showstate(state);

               rbhash[pos_ref]= child;
               rbhash[child_ref]= pos_ref|1; // = parent, colored red.  simplification of ((pos_ref>>1)<<1)|1
               rbhash[parent_ref]= pos^1; // also make pos black
               // rotation finished, exit.
               break;
            }
            state.message.push('Node is Red with Red sibling')
            state.message.push('Pull down Black color from parent')
            state.highlight['node'+(pos_ref>>1)]= 2
            if (showstate) await showstate(state);
            
            rbhash[pos_ref] ^= 1;       // toggle color of pos
            rbhash[pos_ref^1] ^= 1;     // toggle color of sibling
            rbhash[parent_ref] ^= 1;    // toggle color of parent
            // Now pos is black.
            // Jump twice up the tree so that once again, pos has one red child.
            path.pop();
         }
         state.message.push('Tree is balanced')
         state.highlight= {}
         state.finished= true
         if (showstate) await showstate(state)
      }
      return node_id;
   }
   async del(hash_code, cmp_fn, showstate) {
      let rbhash= this.array;
      let bucket= hash_code % this.n_buckets;
      let bucket_ofs= this.table_ofs + bucket;
      let state= {
         message: ["Hash code "+hash_code+" mod "+this.n_buckets+" = "+bucket],
         highlight: { [bucket_ofs]: 'read' },
      };
      if (!rbhash[bucket_ofs]) {
         if (showstate) await showstate(state)
         state.message.push('Bucket is empty')
         state.highlight= {}
         state.finished= true
         if (showstate) await showstate(state)
         return 0
      }
      let at_node= rbhash[bucket_ofs] >> 1
      let path= [ bucket_ofs ]
      let cmp;
      do {
         state.message.push("Compare to node "+at_node);
         state.highlight= { ['node'+at_node]: 2 };
         if (showstate) await showstate(state)
         if (!(cmp= cmp_fn(at_node))) {
            state.message.push("Found");
            break;
         }
         let ref= (at_node<<1) | (cmp < 0? 0 : 1);
         path.push(ref);
         if (path.length > 128) throw "BUG";
         at_node= rbhash[ref] >> 1;
      } while (at_node);
      if (!at_node) {
         state.message.push("Not Found")
         state.highlight= {}
         state.finished= true
         if (showstate) await showstate(state)
         return 0
      }
      
      let pos_ref= path[path.length-1]
      let pos= rbhash[pos_ref]
      // If pos has children, find a leaf to swap with.
      // Then delete this node in the leaf's position.
      let ch1= rbhash[pos], ch2= rbhash[pos ^ 1];
      if (ch1 || ch2) {
         if (ch1 && ch2) {
            state.message.push("Node has two children; need a substitute")
            state.highlight= { ['node'+(ch1>>1)]: 1, ['node'+(ch2>>1)]: 1 }
            if (showstate) await showstate(state)

            let orig_path_len= path.length;
            let tmp, alt= pos, alt2, ref;
            // descend one level to the left
            path.push(ref= (pos >> 1 << 1));
            alt= rbhash[ref]; // either ch1 or ch2, but now we know it's the left one
            // descend as many levels as possible to the right
            while ((alt= rbhash[ref= alt | 1]))
               path.push(ref);
            // 'alt' is the node we swap with.
            alt= rbhash[path[path.length-1]]
            // is there one to the left?
            state.message.push("Swap with node "+(alt>>1))
            if ((alt2= rbhash[alt >> 1 << 1])) {
               state.message.push("Node "+(alt>>1)+" can be replaced by its red child "+(alt2>>1))
               state.highlight= { ['node'+(alt>>1)]: 2, ['node'+(alt2>>1)]: 1, ['node'+(pos>>1)]: 2 }
               if (showstate) await showstate(state)
               // it is required to be a red leaf, so replace alt with it
               rbhash[path[path.length-1]]= alt2 ^ 1
               rbhash[alt2]= 0
               rbhash[alt2 ^ 1]= 0
               // Now substitute this for pos and we're done.
               rbhash[alt | 1]= rbhash[pos | 1]
               rbhash[(alt | 1) ^ 1]= rbhash[(pos | 1) ^ 1]
               rbhash[pos_ref]= (alt >> 1 << 1) | (pos & 1) // preserve color of pos
               pos |= 1 // to avoid entering the balancing loop
            }
            else {
               state.highlight= { ['node'+(alt>>1)]: 2, ['node'+(pos>>1)]: 2 }
               if (showstate) await showstate(state)
               // swap colors of alt and pos
               alt ^= pos & 1;
               pos ^= alt & 1;
               alt ^= pos & 1;
               rbhash[alt | 1]= rbhash[pos | 1];             // copy right
               rbhash[(alt | 1) ^ 1]= rbhash[(pos | 1) ^ 1]; // copy left
               rbhash[pos_ref]= alt;
               // the parent ref at orig_p_i+1 just changed address, so update that
               // (and this affects the next line if alt was a child of pos)
               path[orig_path_len]= alt >> 1 << 1; // was left branch at that point
               pos_ref= path[path.length-1]
               rbhash[pos_ref]= 0 // remove it
            }  
         }
         else {
            state.message.push("Node has one red child, swap with "+((ch1|ch2)>>1))
            state.highlight= { ['node'+((ch1|ch2)>>1)]: 2, ['node'+(pos>>1)]: 1 }
            if (showstate) await showstate(state)
            // Node is black with one child.  Swap with it.
            rbhash[pos_ref]= (ch1 | ch2) >> 1 << 1; // and make it black
            pos |= 1 // to avoid entering the balancing loop
         }
      }
      // Remove it.
      else {
         state.message.push("Node "+(pos_ref>>1)+" is black with no children")
         rbhash[pos_ref]= 0
      }
      // flag for immediate deletion from user_array, for animation
      state.deleted_id= pos >> 1;
      // clean the 'pos' node for future use
      rbhash[pos]= 0;
      rbhash[pos ^ 1]= 0;
      // It was a black node with no children.  Now it gets interesting.
      if (!(pos & 1)) {
         // The tree must have the same number of black nodes along any path from root
         // to leaf.  We want to remove a black node, disrupting the number of black
         // nodes along the path from the root to the current leaf.  To correct this,
         // we must either reduce all other paths, or add a black node to the current
         // path.

         // Loop until the current node is red, or until we get to the root node.
         let sibling= rbhash[pos_ref ^ 1];
         path.pop(); // p_i is now the index of the ref to the parent
         while (path.length) {
            state.message.push("Balance defecit of black node at node "+(pos_ref>>1));
            state.highlight= { ['node'+(pos_ref>>1)]: 2, ['node'+(sibling>>1)]: 1 };
            let near_nephew_ref, near_nephew;
            // If the sibling is red, we are unable to reduce the number of black
            //  nodes in the sibling tree, and we can't increase the number of black
            //  nodes in our tree..  Thus we must do a rotation from the sibling
            //  tree to our tree to give us some extra (red) nodes to play with.
            // This is Case 1 from the text
            if (sibling & 1) {
               // node is black and sibling is red
               state.message.push('Other subtree is red, rotate toward us');
               if (showstate) await showstate(state)
               // get ref to sibling's near subtree
               near_nephew_ref= (sibling ^ 1) | (pos_ref & 1);
               // sibling is new parent, and now black.
               rbhash[path[path.length-1]]= sibling ^ 1;
               // move sibling's child under parent, becoming new sibling (which is black)
               sibling= rbhash[near_nephew_ref];
               rbhash[pos_ref ^ 1]= sibling;
               rbhash[near_nephew_ref]= pos_ref | 1; // former sibling sameside tree = parent, now red
               path.push(near_nephew_ref); // insert new parent into list
            }
            // sibling will be black here

            // If the sibling is black and both children are black, we have to
            //  reduce the black node count in the sibling's tree to match ours.
            // This is Case 2a from the text.
            near_nephew_ref= sibling | (pos_ref & 1);
            near_nephew= rbhash[near_nephew_ref];
            if (!((near_nephew|rbhash[near_nephew_ref ^ 1]) & 1)) {
               state.message.push('Other subtree is black with two black children');
               state.message.push('reduce black count in subtree by changing head to red');
               if (showstate) await showstate(state)
               
               rbhash[pos_ref ^ 1] |= 1; // change sibling to red
               // Now we move one level up the tree to continue fixing the
               // other branches.
               if (path.length <= 1)
                  break;
               pos_ref= path.pop()
               if (rbhash[pos_ref] & 1) {
                  // Now, make the current node black (to fulfill Case 2b)
                  rbhash[pos_ref] ^= 1;
                  break;
               }
               sibling= rbhash[pos_ref ^ 1];
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
               if (near_nephew & 1) {
                  state.message.push('Other subtree is black with a red child on near-side');
                  state.message.push("Rotate subtree's children toward far side");
                  if (showstate) await showstate(state)
                  // Case 3 from the text, double rotation
                  let tmp_ref= near_nephew ^ (pos_ref & 1); // near nephew's far child
                  rbhash[near_nephew_ref]= rbhash[tmp_ref];
                  rbhash[pos_ref ^ 1]= near_nephew;
                  rbhash[tmp_ref]= sibling;
                  sibling= near_nephew ^ 1; // make it black
                  near_nephew_ref= sibling | (pos_ref & 1);
               }
               else {
                  state.message.push('Subtree is black with a red child on far-side. Change it to black,');
               }
               state.message.push('then rotate toward current path, gaining a black node.');
               if (showstate) await showstate(state)
               if (!(near_nephew & 1))
                  rbhash[near_nephew_ref ^ 1] ^= 1; // far nephew becomes black

               // now Case 4 from the text
               rbhash[pos_ref ^ 1]= rbhash[near_nephew_ref];
               // parent becomes black, balancing current path
               rbhash[near_nephew_ref]= pos_ref >> 1 << 1; 
               // Sibling assumes parent's color and position
               rbhash[path[path.length-1]]= sibling | (rbhash[path[path.length-1]] & 1);
               break;
            }
         }
      }
      // store the temp root back into the bucket in case root of tree changed
      state.message.push('Tree is balanced');
      state.highlight= {}
      state.finished= true
      if (showstate) await showstate(state)
      return pos >> 1;
   }
}
