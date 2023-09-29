<script setup>
import { ref } from 'vue'
import RBHashArrayView from './component/RBHashArrayView.vue'
import RBHashVisualizer from './component/RBHashVisualizer.vue'
import RBHash from './RBHash.js'

const user_array= ref([])
const rbhash= ref(new RBHash(8, 8))

const hashtable_size_factor= ref(1.0)
const hash_function= ref('sum')
const enable_step= ref(false);
const op_in_progress= ref(false);
const next_key= ref();
const vis= ref(null);

const messages= ref([]);

function hashfunc(str) {
   return 0;
}

function resize_rbhash(new_capacity) {
   let n_buckets= parseInt(new_capacity * parseFloat(hashtable_size_factor.value));
   let new_rbh= new RBHash(new_capacity, n_buckets);
   new_rbh.batch_insert(
      user_array.value.length,
      (i) => hashfunc(user_array.value[i-1]),
      (i,j) => user_array.value[i-1].localeCompare(user_array.value[j-1])
   )
   rbhash.value= new_rbh
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
         resize_rbhash(rbhash.value.capacity? rbhash.value.capacity*2 : 16);
      let ins_found= await rbhash.value.insert(
         user_array.value.length+1,
         hashfunc(value),
         (j) => value.localeCompare(user_array.value[j-1]),
         (state) => { messages.value= [].concat(state.message); return get_step_promise() }
      );
      if (ins_found == user_array.value.length+1) {
         user_array.value.push(value);
         next_key.value= '';
      }
   }
   catch (e) {
      console.log(e);
   }
   op_in_progress.value= false;
   vis.value.render();
}

function delete_value(value) {
}

resize_rbhash(8);
</script>

<template>
   <div class="app">
      <div>
         <div class="config">
            <label>Hash Function:
               <select name="hash_function">
               <option value="sum">Sum of Characters Mod Buckets</option>
               </select>
            </label>
            <label>Hash Table Size Factor:
               <input name="hashtable_size_factor" type="text" pattern="[0-9]*(\.?[0-9]+)"
                  v-model="hashtable_size_factor" @keyup.enter="resize_rbhash(rbhash.capacity)">
            </label>
         </div>
         <RBHashArrayView :rbhash=rbhash :user_array=user_array />
      </div>
      <div>
         <div class="control">
            <label>Value: <input name="value" :disabled="op_in_progress" v-model="next_key" @keyup.enter="add_value()" /></label>
            <button type="button" :disabled="op_in_progress" @click="add_value()">Add</button>
            <button type="button" :disabled="op_in_progress" @click="delete_value()">Delete</button>
            <br>
            <label>Step-by-Step <input type="checkbox" v-model="enable_step"></label>
            <button type="button" @click="step()" :disabled="!enable_step">Step</button>
         </div>
         <RBHashVisualizer ref="vis" :rbhash=rbhash :user_array=user_array />
         <div>
            <div v-for="line in messages">{{ line }}</div>
         </div>
      </div>
   </div>
</template>

<style scoped>
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
   border: 1px solid yellow;
}
.control {
   display: flex; gap: 8px;
}
.main {
   display: flex; gap: 16px;
}
</style>
