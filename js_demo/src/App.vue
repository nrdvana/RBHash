<!--
RBHash demonstration Vue3 application
Copyright 2023, Michael Conrad
-->
<script setup>
import { ref, triggerRef, watch } from 'vue'
import RBHashArrayView from './component/RBHashArrayView.vue'
import RBHashVisualizer from './component/RBHashVisualizer.vue'
import RBHash from './RBHash.js'

// enables v-focus in templates
const vFocus = {
  mounted: (el) => el.focus()
}

const show_description= ref(true)
const hash_function= ref('sum')
const rbhash_capacity= ref(15);
const rbhash_buckets= ref(16);
const search_key= ref();
const enable_step= ref(false);
const step_promise= ref(null);
const vis= ref(null);
const messages= ref([]);

const user_array= ref([])
const rbhash= ref()
const vis_markup= ref({})

function _build_rbhash() {
   let new_capacity= parseInt(rbhash_capacity.value),
       new_buckets=  parseInt(rbhash_buckets.value),
       new_hash= hash_function.value
   // This gets called by event handlers, so don't recreate unless the params differ
   if (rbhash.value
      && rbhash.value.capacity == new_capacity && rbhash.value.n_buckets == new_buckets
      && rbhash.value.hash_type == new_hash)
      return;
   // Chop off user_array if the user asked to shrink it
   if (user_array.value.length > new_capacity)
      user_array.value.splice(new_capacity, user_array.value.length - new_capacity)
   // create and populate new rbhash
   let new_rbh= new RBHash(new_capacity, new_buckets)
   new_rbh.hash_type= new_hash
   new_rbh.batch_insert(
      user_array.value.length,
      (i) => hashfunc(user_array.value[i-1]),
      (i,j) => user_array.value[i-1].localeCompare(user_array.value[j-1])
   )
   return rbhash.value= new_rbh
}
watch(hash_function, _build_rbhash);
_build_rbhash()

function hashfunc(str) {
   if (str == null)
      return null
   if (hash_function.value == 'sum') {
      let x= 0;
      for (let i= 0; i < str.length; i++)
         x += str.codePointAt(i);
      return x;
   }
   if (hash_function.value == 'md5') {
      // TODO
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
      return str.codePointAt(0) * str.codePointAt(str.length-1);
   }
   return 0;
}

function step() {
   if (step_promise.value) {
      step_promise.value.resolve(true)
      step_promise.value= null
      triggerRef(rbhash)
   }
}
function get_step_promise(state) {
   //console.log('get_step_promise', JSON.stringify(state), JSON.stringify(vis_markup.value), !!step_promise.value)
   if (step_promise.value == null) {
      if (!enable_step.value)
         return null;
      if (state.message)
         messages.value= [].concat(state.message)
      // Allow the callback to communicate that the element has been deleted at this stage
      if (state.deleted_id)
         user_array.value[state.deleted_id-1]= null
      vis_markup.value.highlight= state.highlight
      vis_markup.value.insert_at= state.insert_at
      if (!state.finished) {
         let res, rej;
         step_promise.value= new Promise((resolve, reject) => { res= resolve; rej= reject; });
         step_promise.value.resolve= res;
         step_promise.value.reject= rej;
      }
   }
   //console.log(JSON.stringify(vis_markup.value));
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
      if (user_array.value.length >= rbhash.value.capacity) {
         rbhash_capacity.value= rbhash_capacity.value * 2
         rbhash_buckets.value=  rbhash_buckets.value * 2
         _build_rbhash()
      }
      // insert into user array first so that rendering mid-algorithm doesn't fail
      let node_id;
      for (node_id= 1; node_id <= user_array.value.length; node_id++)
         if (user_array.value[node_id-1] === null)
            break;
      if (node_id > user_array.value.length)
         user_array.value.push(key)
      else
         user_array.value[node_id-1]= key;
      let ins_found= await rbhash.value.insert(
         node_id,
         hashfunc(key),
         (j) => key.localeCompare(user_array.value[j-1]),
         (state) => get_step_promise(state),
      );
      if (ins_found == node_id)
         search_key.value= ''
      else { // remove the item from the array if the insert didn't succeed
         user_array.value[node_id-1]= null
         if (node_id == user_array.value.length)
            user_array.value.pop()
      }
   }
   catch (e) {
      console.log(e);
   }
   step_promise.value= null
   vis.value.render()
}

async function delete_value(value) {
   if (step_promise.value) // prevent multiple entry
      return;
   try {
      let key= search_key.value;
      let node_id= await rbhash.value.del(
         hashfunc(key),
         (j) => key.localeCompare(user_array.value[j-1]),
         (state) => get_step_promise(state),
      );
      if (node_id > 0) {
         user_array.value[node_id-1]= null
         search_key.value= ''
      }
   }
   catch (e) {
      console.log(e)
   }
   step_promise.value= null
   vis.value.render()
}

</script>

<template>
   <h3>Red/Black Hash Table "RBHash" Demonstration</h3>
   <div :class="'description' + (show_description? '' : ' collapse')">
      <p>This is a hash table where each bucket collision is a Red/Black Tree, where the
      Red/Black nodes are reduced to a pair of small integers, and nodes are pre-allocated up
      to a "capacity", which is the maximum number of user data elements before re-allocating
      and rebuilding the structure.
      Whenever the hash table gets rebuilt, the trees get reconstructed as new collisions
      are found.  While there must be one node for each potential data element, the number of
      buckets in the hash table and quality of the hash function are arbitrary, since even a
      bad hash function and tiny hash table have worst case performance of <code>N(log N)</code>.
      </p>
      <p>The Red/Black nodes can be allocated in the same block of memory as the hash
      table, or even the same block of memory as the array holding the user data elements.
      For capacity of 127 or less, the nodes use 2 bytes each; for capacity 0x7FFF or less, the
      nodes use 4 bytes each, and so on.  Beyond 32K elements, the memory savings of the
      structure are likely outweighed by the cost of having to rebuild such a large structure as
      it grows, and you should probably look for other algorithms like Hash-Tries or B-Trees.
      However, if you know the upper bound of your capacity and don't mind allocating that
      up-front, a filled-to-capacity RBHash will still consume less memory than most other
      algorithms that build trees or tries from small allocations.
      </p>
      <p>The compelling features of this structure vs. other contemporary hash tables are that
      it is not vulnerable to collision attacks (intentional or accidental), and supports
      efficient deletion, while using comparable amounts of memory and reconstruction patterns.
      It also allows you to preserve original insertion order, though this depends on how you
      implement deletions in the "user array" (which need not strictly be an array).
      It also doubles as a Red/Black Tree library by setting <code>buckets = 1</code>.
      </p>
      <p>The design of this demo is heavily inspired by
      <a href="http://gauss.ececs.uc.edu/RedBlack/redblack.html">Dr. John Franco's Red/Black Java Applet</a>
      (which no longer runs in browsers, sadly) and is based roughly on the same Red/Black
      algorithm, as described in
      <i>Berman and Paul. Sequential and Parallel Algorithms. 1997 (ISBN:0-534-94674-7)</i>
      although it underwent some fairly drastic changes when I converted it to use an array of
      integers.  This demo is written using <a href="https://vuejs.org/">Vue 3</a>.
      </p>
      <button @click="show_description=false">Close</button>
   </div>
   <div class="app">
      <div style="flex-grow: 0">
         <div class="config">
            <label>Hash Function:
               <select name="hash_function" v-model="hash_function">
               <option value="murmur">MurmurHash32</option>
               <!-- <option value="md5">MD5</option> -->
               <option value="sum">Sum of Characters</option>
               <option value="firstlast">First and Last Character</option>
               <option value="zero">The Number Zero</option>
               </select>
            </label>
            <div>
               <label>Capacity
                  <input class="rbparam" type="text" pattern="[0-9]+" v-model="rbhash_capacity" @keyup.enter="_build_rbhash()">
               </label>
               <label>Buckets
                  <input class="rbparam" type="text" pattern="[0-9]+" v-model="rbhash_buckets"  @keyup.enter="_build_rbhash()">
               </label>
               <button :style="'visibility:'+(rbhash_capacity != rbhash.capacity || rbhash_buckets != rbhash.n_buckets? 'visible':'hidden')"
                  @click="_build_rbhash()">
                  Apply
               </button>
            </div>
         </div>
         <div class="arrayview-container">
            <RBHashArrayView :rbhash=rbhash :user_array=user_array :markup=vis_markup
               @node-click="(e) => select_node(e.node_id)"
            />
         </div>
      </div>
      <div style="flex-grow: 1">
         <div class="ops">
            <label>Key: <input ref="search_blank" :disabled="step_promise" v-focus v-model="search_key" @keyup.enter="add_value()" /></label>
            <button type="button" :disabled="step_promise" @click="add_value()">Add</button>
            <button type="button" :disabled="step_promise" @click="delete_value()">Delete</button>
            <br>
            <label>Step Through Algorithm <input type="checkbox" v-model="enable_step"></label>
            <button type="button" @click="step()" :style="'visibility:'+(step_promise? 'visible':'hidden')">Step</button>
         </div>
         <div style="border: 1px solid black; background-color: white;">
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
.description { margin: 4px; box-shadow: 1px 2px 3px; padding: 8px; background-color: white; }
.description p { margin-top: 0; }
.description.collapse { display: none; }
.app {
   min-width: 800px;
   min-height: 400px;
   padding: 16px;
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
.config .rbparam { width: 3em; text-align: center; margin-right: .5em; }
.control {
   display: flex; gap: 8px;
}
.ops button { margin: 0 0 0 .5em; }
.arrayview-container {
   overflow-y: scroll;
}
.main {
   display: flex; gap: 16px;
}
</style>
