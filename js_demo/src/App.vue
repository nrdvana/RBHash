<script setup>
import { ref, watch } from 'vue'
import RBHashArrayView from './component/RBHashArrayView.vue'
import RBHashVisualizer from './component/RBHashVisualizer.vue'
import RBHash from './RBHash.js'

const show_description= ref(true)
const hashtable_size_factor= ref(1.0)
const hash_function= ref('sum')
const enable_step= ref(false);
const search_key= ref();
const vis= ref(null);
const step_promise= ref(null);
const messages= ref([]);

const user_array= ref([])
const rbhash= ref(recreate_rbhash(15))
const vis_markup= ref({})

watch([ hashtable_size_factor, hash_function ], () => {
   rbhash.value= recreate_rbhash(rbhash.value.capacity)
});

function hashfunc(str) {
   if (hash_function.value == 'sum') {
      let x= 0;
      for (let i= 0; i < str.length; i++)
         x += str.codePointAt(i);
      return x;
   }
   if (hash_function.value == 'murmur') {
      // This is (mostly) MurmurHash32 by Austin Appleby, simplified for JavaScript
      // https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp#L94
      let hash= 0;
      let bytes= new Array(str.length);
      for (let i= 0; i < str.length; i++) {
         let cp= str.codePointAt(i);
         while (cp) {
            bytes.push(cp & 0xFF);
            cp >>= 8;
         }
      }
      while (bytes.length & 3)
         bytes.push(0);
      for (let i= 0; i < bytes.length / 4; i++) {
         let k1= (bytes[i+3] << 24) | (bytes[i+2] << 16)
            | (bytes[i+1] << 8) | bytes[i];
         k1 *= 0xcc9e2d51;
         k1 = (k1 << 15) | (k1 >> 17);
         k1 *= 0x1b873593;
         hash ^= k1;
         hash= (hash << 13) | (hash >> 19);
         hash= hash * 5 + 0xe6546b64;
      }
      hash ^= str.length;
      hash ^= hash >> 16;
      hash *= 0x85ebca6b;
      hash ^= hash >> 13;
      hash *= 0xc2b2ae35;
      hash ^= hash >> 16;
      return hash;
   }
   if (hash_function.value == 'firstlast') {
      return (str.codePointAt(0) << 8) ^ str.codePointAt(str.length-1);
   }
   return 0;
}

function recreate_rbhash(new_capacity) {
   let n_buckets= parseInt(new_capacity * parseFloat(hashtable_size_factor.value));
   let new_rbh= new RBHash(new_capacity, n_buckets);
   new_rbh.batch_insert(
      user_array.value.length,
      (i) => hashfunc(user_array.value[i-1]),
      (i,j) => user_array.value[i-1].localeCompare(user_array.value[j-1])
   )
   return new_rbh;
}

function step() {
   if (step_promise.value) {
      step_promise.value.resolve(true);
      step_promise.value= null;
      vis.value.render();
   }
}
function get_step_promise(state) {
   console.log('get_step_promise', JSON.stringify(state), JSON.stringify(vis_markup.value), !!step_promise.value)
   if (step_promise.value == null) {
      if (!enable_step.value)
         return null;
      if (state.message)
         messages.value= [].concat(state.message)
      if (state.highlight)
         vis_markup.value.highlight= state.highlight
      if (state.insert_at)
         vis_markup.value.insert_at= state.insert_at
      if (!state.finished) {
         let res, rej;
         step_promise.value= new Promise((resolve, reject) => { res= resolve; rej= reject; });
         step_promise.value.resolve= res;
         step_promise.value.reject= rej;
      }
   }
   console.log(JSON.stringify(vis_markup.value));
   return step_promise.value;
}

function select_node(node_id) {
   // Update magic variable to let visualizations show it
   if (vis_markup.value.selected_node != node_id) {
      vis_markup.value.selected_node= node_id
      if (node_id != null)
         // Update key blank with the value of this array element
         if (node_id > 0 && node_id <= user_array.value.length && search_key.value != user_array.value[node_id-1])
            search_key.value= user_array.value[node_id-1]
   }
}

// every time search_key changes, look for that element in the array
watch(search_key, () => {
   // would use rbtree algorithm, except the tree might be mid-change and not valid
   for (let i=0; i < user_array.value.length; i++)
      if (user_array.value[i] == search_key.value) {
         select_node(i+1)
         return
      }
   select_node(null)
})

async function add_value() {
   if (step_promise.value) // prevent multiple entry
      return;
   try {
      let key= search_key.value;
      if (user_array.value.length >= rbhash.value.capacity)
         rbhash.value= recreate_rbhash(rbhash.value.capacity? rbhash.value.capacity*2 : 16);
      // insert into user array first so that rendering mid-algorithm doesn't fail
      user_array.value.push(key);
      let ins_found= await rbhash.value.insert(
         user_array.value.length,
         hashfunc(key),
         (j) => key.localeCompare(user_array.value[j-1]),
         (state) => get_step_promise(state),
      );
      if (ins_found == user_array.value.length)
         search_key.value= ''
      else // remove the item from the array if the insert didn't succeed
         user_array.value.pop();
   }
   catch (e) {
      console.log(e);
   }
   step_promise.value= null
   vis.value.render()
}

function delete_value(value) {
   if (step_promise.value) // prevent multiple entry
      return;
}

</script>

<template>
   <h3>Red/Black HashTable Demonstration</h3>
   <div :class="'description' + (show_description? '' : ' collapse')">
      <p>This is a hash table where each bucket collision is a Red/Black Tree, where the Red/Black
      nodes are reduced to a pair of small integers, and nodes are pre-allocated up to
      the "capacity" of the hash table.  (capacity meaning the maximum number of user
      data elements before you decide to re-allocate the hash table)
      Whenever the hash table gets rebuilt, the trees get reconstructed as new collisions
      are found.  While there must be one node for each potential data element, the size of the
      hashtable and quality of the hash function are arbitrary, since even a bad hash function
      and tiny hash table have worst case performance of <code>N(log N)</code>.
      </p>
      <p>The Red/Black nodes can be allocated in the same block of memory as the hash
      table, or even the same block of memory as the array holding the user data elements.
      For capacity of 126 or less, the nodes use 2 bytes each.  For capacity of 0x7FFE (32K)
      or less, the nodes use 4 bytes each.  For capacity of 0x7FFFFFFE (2G) or less, the nodes
      use 8 bytes each, and so on.  It seems unlikely that anyone would want to allocate
      space for more than that in a single slab of memory, so this algorithm seems primarily
      useful for arrays smaller than 32K elements.  It's also primarily useful for low-level
      languages where you can actually allocte a contiguous array of small integers; this
      JavaScript implementation is only for demonstration purposes.
      </p>
      <button @click="show_description=false">Close</button>
   </div>
   <div class="app">
      <div style="flex-grow: 0">
         <div class="config">
            <label>Hash Function:
               <select name="hash_function" v-model="hash_function">
               <option value="murmur">MurmurHash32</option>
               <option value="sum">Sum of Characters</option>
               <option value="firstlast">First and Last Character</option>
               <option value="zero">The Number Zero</option>
               </select>
            </label>
            <label>Hash Table Size Factor:
               <input name="hashtable_size_factor" type="text" pattern="[0-9]*(\.?[0-9]+)"
                  v-model="hashtable_size_factor" @keyup.enter="rbhash= recreate_rbhash(rbhash.capacity)">
            </label>
         </div>
         <RBHashArrayView :rbhash=rbhash :user_array=user_array :markup=vis_markup />
      </div>
      <div style="flex-grow: 1">
         <div class="ops">
            <label>Key: <input name="value" :disabled="step_promise" v-model="search_key" @keyup.enter="add_value()" /></label>
            <button type="button" :disabled="step_promise" @click="add_value()">Add</button>
            <button type="button" :disabled="step_promise" @click="delete_value()">Delete</button>
            <br>
            <label>Step Through Algorithm <input type="checkbox" v-model="enable_step"></label>
            <button type="button" @click="step()" :style="'visibility:'+(step_promise? 'visible':'hidden')">Step</button>
         </div>
         <div style="border: 1px solid black">
            <RBHashVisualizer ref="vis" :rbhash=rbhash :user_array=user_array :markup=vis_markup
               @node-click="(e) => select_node(e.node_id)"
            />
         </div>
         <div>
            <div v-for="line in messages">{{ line }}</div>
         </div>
      </div>
   </div>
</template>

<style scoped>
h3 { margin: 8px; padding: 0px; }
.description { margin: 4px; box-shadow: 1px 2px 3px; padding: 8px; }
.description p { margin-top: 0; }
.description.collapse { display: none; }
.app {
   min-width: 800px;
   margin: 0 auto;
   min-height: 400px;
   padding: 1em;
   display: flex;
   gap: 8px;
   align-items: stretch;
}
.app > div {
   display: flex;
   flex-direction: column;
   gap: 8px;
}
.config {
   display: flex;
   flex-direction: column;
}
.control {
   display: flex; gap: 8px;
}
.ops button { margin: 0 0 0 .5em; }
.main {
   display: flex; gap: 16px;
}
</style>
