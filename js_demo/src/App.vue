<script setup>
import { ref, watch } from 'vue'
import RBHashArrayView from './component/RBHashArrayView.vue'
import RBHashVisualizer from './component/RBHashVisualizer.vue'
import RBHash from './RBHash.js'

const show_description= ref(true)
const hashtable_size_factor= ref(1.0)
const hash_function= ref('sum')
const enable_step= ref(false);
const op_in_progress= ref(false);
const next_key= ref();
const vis= ref(null);

const messages= ref([]);

const user_array= ref([])
const rbhash= ref(recreate_rbhash(15))

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

let step_promise;
function step() {
   if (step_promise) {
      step_promise.resolve(true);
      step_promise= null;
      vis.value.render();
   }
}
function get_step_promise() {
   if (step_promise == null) {
      if (!enable_step.value)
         return null;
      let res, rej;
      step_promise= new Promise((resolve, reject) => { res= resolve; rej= reject; });
      step_promise.resolve= res;
      step_promise.reject= rej;
   }
   return step_promise;
}

async function add_value() {
   if (step_promise) // prevent multiple entry
      return;
   op_in_progress.value= true;
   try {
      let value= next_key.value;
      if (user_array.value.length >= rbhash.value.capacity)
         rbhash.value= recreate_rbhash(rbhash.value.capacity? rbhash.value.capacity*2 : 16);
      // insert into user array first so that rendering mid-algorithm doesn't fail
      user_array.value.push(value);
      let ins_found= await rbhash.value.insert(
         user_array.value.length,
         hashfunc(value),
         (j) => value.localeCompare(user_array.value[j-1]),
         (state) => { messages.value= [].concat(state.message); return get_step_promise() }
      );
      if (ins_found == user_array.value.length)
         next_key.value= ''
      else // remove the item from the array if the insert didn't succeed
         user_array.value.pop();
   }
   catch (e) {
      console.log(e);
   }
   op_in_progress.value= false;
   vis.value.render();
}

function delete_value(value) {
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
               <option value="zero">The Number Zero</option>
               </select>
            </label>
            <label>Hash Table Size Factor:
               <input name="hashtable_size_factor" type="text" pattern="[0-9]*(\.?[0-9]+)"
                  v-model="hashtable_size_factor" @keyup.enter="rbhash= recreate_rbhash(rbhash.capacity)">
            </label>
         </div>
         <RBHashArrayView :rbhash=rbhash :user_array=user_array />
      </div>
      <div style="flex-grow: 1">
         <div>
            <label>Key: <input name="value" :disabled="op_in_progress" v-model="next_key" @keyup.enter="add_value()" /></label>
            <button type="button" :disabled="op_in_progress" @click="add_value()">Add</button>
            <button type="button" :disabled="op_in_progress" @click="delete_value()">Delete</button>
            <br>
            <label>Step-by-Step <input type="checkbox" v-model="enable_step"></label>
            <button type="button" @click="step()" :disabled="!enable_step">Step</button>
         </div>
         <div style="border: 1px solid black">
            <RBHashVisualizer ref="vis" :rbhash=rbhash :user_array=user_array />
         </div>
         <div>
            <div v-for="line in messages">{{ line }}</div>
         </div>
      </div>
   </div>
</template>

<style scoped>
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
.main {
   display: flex; gap: 16px;
}
</style>
