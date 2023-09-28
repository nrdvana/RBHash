<script setup>
import { ref } from 'vue'
import RBHashArrayView from './component/RBHashArrayView.vue'
import RBHashVisualizer from './component/RBHashVisualizer.vue'

const demo= ref({
   hashtable_size_factor: 1.0,
   hash_function: 'sum',
   rbhash: {
      count: 0,
      capacity: 0,
      array: [],
   },
   visual_cues: {
      highlight: {},
      incoming_value: null,
      incoming_position: null,
      algorithm_comment: null,
   },
});

const value= ref();

function resize_rbhash(new_capacity) {
   let rbhash= demo.value.rbhash;
   let olda= rbhash.array;
   let newa= [];
   let n_buckets= parseInt(new_capacity * demo.value.hashtable_size_factor);
   if (!n_buckets) n_buckets= 1;
   for (let i= 0, lim= rbhash.count; i < lim; i++)
      newa.push(olda[i]);
   for (let i= rbhash.count, lim= new_capacity; i < lim; i++)
      newa.push(null);
   for (let i= 0, lim= new_capacity*2+3 + n_buckets; i < lim; i++)
      newa.push(0);
   demo.value.rbhash.array= newa;
   demo.value.rbhash.capacity= new_capacity;
}

function add_value(value) {
   let rbhash= demo.value.rbhash;
   if (rbhash.count >= rbhash.capacity)
      resize_rbhash(rbhash.capacity? rbhash.capacity*2 : 16);
   rbhash.array[ rbhash.count++ ]= value;
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
               <input name="hashtable_size_factor" type="text" pattern="-?[0-9.]+" v-model="demo.hashtable_size_factor" @keyup.enter="resize_rbhash(demo.rbhash.capacity)">
            </label>
         </div>
         <RBHashArrayView :rbhash=demo.rbhash :visual_cues=demo.visual_cues />
      </div>
      <div>
         <div class="control">
            <label>Value: <input name="value" v-model="value" @keyup.enter="add_value(value)" /></label>
            <button type="button" @click="add_value(value)">Add</button>
            <button type="button" @click="delete_value(value)">Delete</button>
         </div>
         <RBHashVisualizer :rbhash=demo.rbhash :visual_cues=demo.visual_cues />
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
