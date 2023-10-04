<script setup>
const props= defineProps({
   rbhash: Object,
   user_array: Object,
   markup: Object,
})

function nodeused(i) {
   return props.user_array[i-1] != null;
}
function noderef(i, right) {
   return props.rbhash.array[ i*2 + right ] >> 1;
}
function noderef_is_red(i, right) {
   return props.rbhash.array[ i*2 + right ] & 1;
}
function n_buckets() {
   return props.rbhash.n_buckets;
}
function bucket(i) {
   return props.rbhash.array[ props.rbhash.table_ofs + i ] >> 1;
}

</script>

<template>
<table>
<caption>
   {{ rbhash.capacity }} + 1 nodes for user array &amp; Sentinel<br>
   + {{ rbhash.array.length - rbhash.table_ofs }} hash buckets<br>
   = {{ rbhash.array.length * ( rbhash.capacity <= 0x7E? 1 : rbhash.capacity <= 0x7FFE? 2 : 4 ) }} bytes
</caption>
<tr>
   <th>R/B Node Array</th>
   <th>User Array</th>
</tr>
<tr class="first-data-row">
   <td class="node">
      <div class="node-id"><span>(sentinel)&nbsp;</span> 0</div>
      <div class="noderefs">
         <div class="noderef">0</div>
         <div class="noderef">0</div>
      </div>
   </td>
   <td class="user-el"></td>
</tr>
<tr v-for="i in rbhash.capacity">
   <td :class="'node ' + (nodeused(i)? 'full' : 'empty') + (markup.selected_node == i? ' selected' : '') ">
      <div class="node-id">{{ i }}</div>
      <div class="noderefs">
         <div :class="'noderef ' + (!nodeused(i)? 'empty' : !noderef(i,0)? '' : noderef_is_red(i, 0)? 'red' : 'black') ">
            {{ noderef(i, 0) }}
         </div>
         <div :class="'noderef ' + (!nodeused(i)? 'empty' : !noderef(i,1)? '' : noderef_is_red(i, 1)? 'red' : 'black') ">
            {{ noderef(i, 1) }}
         </div>
      </div>
   </td>
   <td :class="'user-el ' + (nodeused(i)? 'full' : 'empty') + (markup.selected_node == i? ' selected' : '') ">
      {{ nodeused(i)? '"' + user_array[i-1] + '"' : '-' }}
   </td>
</tr>
<tr><th>Hash Table</th></tr>
<tr v-for="i in parseInt((n_buckets() + 3)/4)">
   <td class="bucket">
      <template v-for="j in 4">
         <div v-if="(i-1)*4+j-1 < n_buckets()" :class="'noderef ' + (bucket((i-1)*4+j-1)? 'black' : 'empty')">
            {{ bucket((i-1)*4+j-1) }}
         </div>
      </template>
   </td>
   <td class="user-el blank">&nbsp;</td>
</tr>
</table>
</template>

<style scoped>
table { border-collapse: collapse; width: auto; min-width: 200px; width: 200px; }
caption { text-align: left; padding: 8px 0; font-size: 90%; }
th { text-align: left; min-width: 8em; font-family: sans-serif; text-decoration: underline; }
th:first-child { text-align: right; padding-right: 20px; }
td:first-child { padding: 0 20px 0 0; }
td.blank { visibility: hidden; }
.user-el { width: auto; padding: 4px; }
.user-el.selected {
   color: #440;
   background-image: linear-gradient(to left, #FFF, #FFA);
}
.node, .bucket {
   min-width: 6em;
   display: flex; justify-content: flex-end; gap: 8px;
}
.node.selected {
   color: #440;
   font-weight: bold;
   background-image: linear-gradient(to right, #FFF, #FFA);
}
.node-id {
   font-family: monospace, sans-serif;
   min-width: 4em;
   display: flex; align-items: center; justify-content: flex-end;
   /*border-bottom: 1px solid #EEE;*/
   border-top: 1px solid #CCC;
}
.node-id span { font-size: 80%; }
tr.first-data-row .node-id { border-top: none; }
.node.empty { opacity: .3; }
.noderefs {
   display: flex; flex-direction: column;
}
.noderef {
   width: 2em; text-align: right;
   font-family: "Courier New", monospace, sans-serif;
   font-size: 10px;
   font-weight: bold;
   padding: 0px 8px;
   border: 1px solid #DDD;
}
.noderef.red { border: 1px solid red; background-color: #FEE; color: red; }
.noderef.black, .bucket.full { border: 1px solid black; background-color: #333; color: #EEE; }
.noderef.empty { }
   
</style>
