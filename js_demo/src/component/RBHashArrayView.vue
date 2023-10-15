<!--
RBHash byte inspector Vue3 component
Copyright 2023, Michael Conrad
-->
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
   return props.rbhash.array[ i*2 + right ];
}
function noderef_is_red(i, right) {
   return props.rbhash.array[ i*2 + right ] & 1;
}
function n_buckets() {
   return props.rbhash.n_buckets;
}
function bucket(i) {
   return props.rbhash.array[ props.rbhash.table_ofs + i ];
}

const emit= defineEmits([ 'nodeClick' ])

</script>

<template>
   <table>
   <tr>
      <th>RBHash Integers</th>
      <th>User Array</th>
   </tr>
   <tr class="header-comment">
      <td>{{ rbhash.array.length }} {{ rbhash.capacity <= 0x7E? 'bytes' : rbhash.capacity <= 0x7FFE? 'uint16_t' : 'uint32_t' }}
         <div class="tooltip">
         = 2 * ( {{ rbhash.capacity }} + 1 nodes ) + {{ rbhash.array.length - rbhash.table_ofs }} hash buckets
         </div>
      </td>
      <td>{{ rbhash.capacity }} pointers</td>
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
   <tr v-for="i in rbhash.capacity" :class="(markup.selected_node == i? 'selected' : '')" @click="emit('nodeClick', { node_id: i })">
      <td :class="'node ' + (nodeused(i)? 'full' : 'empty') + (markup.selected_node == i? ' selected' : '') ">
         <div class="node-id">{{ i }}</div>
         <div class="noderefs">
            <div :class="'noderef ' + (!nodeused(i)? 'empty' : !noderef(i,0)? '' : noderef_is_red(i, 0)? 'red' : 'black') ">
               {{ noderef(i, 0) }} <div v-if="noderef(i, 0)" class="tooltip">( {{ noderef(i, 0) >> 1 }} &lt;&lt; 1 ) | {{ noderef(i, 0) & 1 }}</div>
            </div>
            <div :class="'noderef ' + (!nodeused(i)? 'empty' : !noderef(i,1)? '' : noderef_is_red(i, 1)? 'red' : 'black') ">
               {{ noderef(i, 1) }} <div v-if="noderef(i, 1)" class="tooltip">( {{ noderef(i, 1) >> 1 }} &lt;&lt; 1 ) | {{ noderef(i, 1) & 1 }}</div>
            </div>
         </div>
      </td>
      <td :class="'user-el ' + (nodeused(i)? 'full' : 'empty') + (markup.selected_node == i? ' selected' : '') ">
         {{ nodeused(i)? '"' + user_array[i-1] + '"' : '-' }}
      </td>
   </tr>
   <tr class="header-comment"><td>Hash Buckets</td></tr>
   <tr v-for="i in parseInt((n_buckets()+3)/4)">
      <td class="bucket">
         <div class="bucket-id">{{ (i-1)*4 }}</div>
         <div class="noderefs">
            <div v-for="j in Math.min(n_buckets()-(i-1)*4, 4)"
               :class="'noderef ' + (bucket((i-1)*4+j-1)? 'black' : 'empty')">
               {{ bucket((i-1)*4+j-1) }} <div v-if="bucket((i-1)*4+j-1)" class="tooltip">{{ bucket((i-1)*4+j-1) >> 1 }} &lt;&lt; 1</div>
            </div>
         </div>
      </td>
      <td class="user-el blank">&nbsp;</td>
   </tr>
   </table>
</template>

<style scoped>
table { border-collapse: collapse; width: auto; min-width: 200px; width: 200px; position: relative; }
caption { text-align: left; padding: 8px 0; font-size: 90%; }
th { text-align: left; min-width: 8em; font-family: sans-serif; text-decoration: underline; }
th:first-child { text-align: right; padding-right: 20px; }
td:first-child { text-align: right; padding: 0 20px 0 0; }
td.blank { visibility: hidden; }
tr.header-comment td { font-size: 70%; }
.tooltip {
   position: absolute;
   visibility: hidden;
   padding: 4px;
   color: black;
   background-color: #FFD;
   box-shadow: 1px 2px 2px;
}
*:hover > .tooltip { visibility: visible; }
.user-el { width: auto; padding: 4px; }
.user-el.selected {
   color: #440;
   background-image: linear-gradient(to left, #FFFFFF00, #FF9);
}
.node, .bucket {
   min-width: 6em;
   display: flex; justify-content: flex-end; gap: 8px;
}
.node.selected {
   color: #440;
   font-weight: bold;
   background-image: linear-gradient(to right, #FFFFFF00, #FF9);
}
.node-id {
   font-family: monospace, sans-serif;
   min-width: 4em;
   display: flex; align-items: center; justify-content: flex-end;
   /*border-bottom: 1px solid #EEE;*/
   border-top: 1px solid #CCC;
}
.node-id span { font-size: 80%; }
.bucket-id { font-size: 10px; }
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
