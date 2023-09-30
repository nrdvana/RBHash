export default class RBHash {
   constructor(capacity, n_buckets) {
      this.capacity= capacity;
      this.n_buckets= n_buckets > 1? n_buckets : 1;
      this.root_ref_ofs= capacity*2 + 2;
      this.table_ofs= this.root_ref_ofs + 1;
      this.array= new Array(this.table_ofs + n_buckets);
      this.array.fill(0);
   }
   batch_insert(n_values, hash_fn, cmp_fn) {
      for (let i= 1; i <= n_values; i++)
         this.insert(i, hash_fn(i), (i2) => cmp_fn(i, i2), null);
   }
   async insert(node_id, hash_code, cmp_fn, showstate) {
      let rbhash= this.array;
      let bucket= hash_code % this.n_buckets;
      let bucket_ofs= this.table_ofs + bucket;
      let state= {
         message: ["Hash code "+hash_code+" mod "+this.n_buckets+" = "+bucket],
         highlight: { [bucket_ofs]: 'test' },
         newnode_at: bucket_ofs
      };
      if (!rbhash[bucket_ofs]) {
         if (showstate) await showstate(state)
         rbhash[bucket_ofs]= node_id;
         state.finished= true;
         state.highlight[bucket_ofs]= 'write';
         if (showstate) await showstate(state)
      }
      else {
         let at_node= rbhash[bucket_ofs]
         rbhash[ this.root_ref_ofs ]= at_node << 1;
         let path= [ this.root_ref_ofs ]
         let cmp;
         state.message.push('Bucket is full...')
         do {
            state.message.push("Compare to node "+at_node);
            state.newnode_at= at_node;
            if (showstate) await showstate(state)
            if (!(cmp= cmp_fn(at_node))) {
               state.message.push("Found a duplicate, will not insert");
               state.finished= true;
               if (showstate) await showstate(state)
               return node;
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
         state.highlight[pos_ref]= 'write';
         state.newnode_at= null;
         state.cur_node= pos_ref>>1;
         if (showstate) await showstate(state);
         
         while (path.length > 1) {
            let pos_ref= path.pop();
            let pos= rbhash[pos_ref];
            let parent_ref= path[path.length-1];
            state.message.push('Balance at node '+(pos>>1));
            state.cur_node= pos_ref>>1;
            // if current is a black node, no rotations needed
            if (!(pos & 1)) {
               state.message.push('Node is Black, no rotations needed.');
               break;
            }
            // pos is red, its new child is red, and parent will be black.
            // if the sibling is also red, we can pull down the color black from the parent
            // if not, need a rotation.
            if (!(rbhash[pos_ref^1] & 1)) {
               // Sibling is black, need a rotation.
               state.message.push('Node is Red with Black sibling. Need a rotation.')
               // if the imbalanced child (red node) is on the same side as the parent,
               //  need to rotate those lower nodes to the opposite side in preparation
               //  for the rotation.
               // e.g. if pos_ref is leftward (even) and pos's rightward child (odd) is the red one...
               let child_ref= pos ^ (pos_ref&1);
               let child= rbhash[child_ref];
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
            state.highlight[pos_ref]= 'write';
            state.highlight[pos_ref^1]= 'write';
            state.highlight[parent_ref]= 'write';
            if (showstate) await showstate(state);
            
            rbhash[pos_ref] ^= 1;       // toggle color of pos
            rbhash[pos_ref^1] ^= 1;     // toggle color of sibling
            rbhash[parent_ref] ^= 1;    // toggle color of parent
            // Now pos is black.
            // Jump twice up the tree so that once again, pos has one red child.
            path.pop();
         }
         state.message.push('Tree is balanced');
         state.finished= true;
         if (showstate) await showstate(state);
         // store the temp root back into the bucket in case root of tree changed
         rbhash[bucket_ofs]= rbhash[ this.root_ref_ofs ] >> 1;
         rbhash[ this.root_ref_ofs ]= 0;
      }
      return node_id;
   }
}
