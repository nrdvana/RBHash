<script setup>
const props= defineProps({
   rbhash: Object,
   visual_cues: Object,
})

function nodeused(i) {
   return props.rbhash.array[i-1] != null;
}
function noderef(i, right) {
   return props.rbhash.array[ props.rbhash.capacity + i*2 + right ] >> 1;
}
function noderef_is_red(i, right) {
   return props.rbhash.array[ props.rbhash.capacity + i*2 + right ] & 1;
}
function rootref() {
   return props.rbhash.array[ props.rbhash.capacity*2 + 3 ];
}
function n_buckets() {
   return props.rbhash.array.length - (props.rbhash.capacity*3+3);
}
function bucket(i) {
   return props.rbhash.array[ props.rbhash.capacity*3 + 3 + i ];
}

</script>

<template>
<table>
<caption>
   {{ rbhash.capacity }} element capacity,<br>
   {{ rbhash.capacity+1 }} R/B nodes + 1 temporary root-ref,<br>
   {{ (rbhash.array.length - (rbhash.capacity*3+3)) }} hash buckets<br>
</caption>
<tr>
   <th>R/B Hash Array</th>
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
   <td :class="'node ' + (nodeused(i)? 'full' : 'empty') ">
      <div class="node-id">{{ i }}</div>
      <div class="noderefs">
         <div :class="'noderef ' + (!nodeused(i)? 'empty' : !noderef(i,0)? '' : noderef_is_red(i, 0)? 'red' : 'black') ">
            {{ noderef(i, 0) }}
         </div>
         <div :class="'noderef ' + (!nodeused(i)? 'empty' : !noderef(i,0)? '' : noderef_is_red(i, 1)? 'red' : 'black') ">
            {{ noderef(i, 1) }}
         </div>
      </div>
   </td>
   <td :class="'user-el ' + (nodeused(i)? 'full' : 'empty') ">
      {{ nodeused(i)? '"' + rbhash.array[ i - 1 ] + '"' : '-' }}
   </td>
</tr>
<tr>
   <td :class=" 'node root-ref ' + (rootref()? 'full' : 'empty') ">
      <div class="node-id"><span>tmp root</span></div>
      <div class="noderefs">
         <div :class="'noderef ' + (rootref()? 'black' : 'empty') ">
            {{ rootref() }}
         </div>
         <div class="noderef" style="visibility:hidden">&nbsp;</div>
      </div>
   </td>
   <td class="user-el blank">&nbsp;</td>
</tr>
<tr><th colspan=2>Hash Table</th></tr>
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
td { padding: 0 12px; }
td.blank { visibility: hidden; }
.user-el { width: auto; padding: 4px; }
.node, .bucket {
   min-width: 6em;
   display: flex; justify-content: flex-end; gap: 8px;
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