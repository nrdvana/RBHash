<script setup>
import { ref, watch, onMounted } from 'vue'

const props= defineProps({
   rbhash: Object,
   user_array: Object,
   node_size: Number
})

let old_layout= {};
let new_layout= {};

function calc_subtree_layout(rbhash, nodes, node_id) {
   let node= nodes[node_id]
   node.width= 1;
   node.height= 1;
   for (let subtree= 0; subtree <= 1; subtree++) {
      let ref= rbhash.array[node_id*2+subtree]
      if (ref) {
         node[subtree]= calc_subtree_layout(rbhash, nodes, ref>>1)
         node[subtree].parent_id= node_id;
         if (ref & 1) node[subtree].red= 1
         node.width += node[subtree].width
         node.height= Math.max(node.height, 1 + node[subtree].height);
      }
   }
   return node;
}

function calc_layout(rbhash, user_array) {
   let table= {};
   let nodes= new Array(user_array.length+1);
   for (let node_id= 1; node_id <= user_array.length; node_id++) {
      nodes[node_id]= { id: node_id, key: user_array[node_id-1] };
   }
   // For each filled bucket, count number of nodes in that tree
   let width= 0;
   for (let b= 0; b < rbhash.n_buckets; b++) {
      let node_id= rbhash.array[rbhash.table_ofs + b];
      if (node_id) {
         let node= calc_subtree_layout(rbhash, nodes, node_id)
         table[b]= { node: node }
         width += node.width;
      }
   }
   let node_diam= parseFloat(props.node_size || 20);
   return {
      table,
      nodes,
      n_buckets: rbhash.n_buckets,
      node_width_sum: width,
      node_rad: node_diam / 2,
      node_dx: node_diam + 2,
      node_dy: node_diam + 2,
   }
}

function render_subtree(layout, node, x, y, ctx) {
   node.x= x + layout.node_dx * (node[0]? node[0].width:0) + layout.node_dx/2;
   node.y= y + layout.node_dy/2;
   if (node.parent_id) {
      let parent= layout.nodes[node.parent_id];
      ctx.lineWidth= 1;
      ctx.strokeStyle= node.red? "red" : "black";
      ctx.beginPath(); ctx.moveTo(node.x, node.y); ctx.lineTo(parent.x, parent.y); ctx.stroke();
   }
   if (node[0]) render_subtree(layout, node[0], x, y + layout.node_dy, ctx);
   if (node[1]) render_subtree(layout, node[1], node.x + layout.node_dx/2, y + layout.node_dy, ctx);
   ctx.lineWidth= 1;
   ctx.strokeStyle= node.red? "red" : "black";
   ctx.beginPath()
   ctx.arc(node.x, node.y, layout.node_rad, 0, 2*Math.PI)
   ctx.fillStyle= node.red? '#FDD' : '#EEE';
   ctx.fill();
   ctx.stroke();
   ctx.fillStyle= node.red? 'red':'black';
   ctx.textAlign = 'center'
   ctx.textBaseline = 'middle'
   ctx.fillText(node.id, node.x, node.y);
}

function render_layout(layout, ctx, canvas_width) {
   let node_dx= layout.node_dx
   // make a list of the widest buckets
   let widest= Object.values(layout.table)
      .map((bucket) => bucket.node.width * node_dx)
      .sort()
   let widest_sum= widest.reduce((ret, cur) => ret + cur, 0);
   let standard_bucket_count= layout.n_buckets - widest.length
   let standard_bucket_dx;
   // Does the minimal width of all the nodes occupy all the space?
   if (widest_sum >= canvas_width) {
      standard_bucket_dx= 3
   } else {
      // for each small node, can we make all buckets at least this large without running out of room?
      while (widest.length && (widest_sum - widest[0]) + (widest[0] * standard_bucket_count) <= canvas_width) {
         console.log(widest_sum, widest.concat([]), standard_bucket_count)
         // convert least-wide to a standard bucket width
         widest_sum -= widest[0]
         widest.shift();
         standard_bucket_count++
      }
         console.log(widest_sum, widest.concat([]), standard_bucket_count)
      // divide remaiing space among the standard buckets
      standard_bucket_dx= (canvas_width - widest_sum) / standard_bucket_count;
   }
   // render each filled bucket, first rendering the empty buckets to the left.
   let x= 0
   for (let b= 0; b < layout.n_buckets; b++) {
      // bucket left edge
      ctx.lineWidth= 1
      ctx.strokeStyle= '#333'
      ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, 20); ctx.stroke();
      let dx;
      // bucket tree
      if (layout.table[b]) {
         let node= layout.table[b].node
         let tree_dx= node_dx * node.width
         dx= Math.max(tree_dx, standard_bucket_dx)
         render_subtree(layout, node, x + (dx - tree_dx)/2, 15, ctx)
      } else {
         dx= standard_bucket_dx
      }
      // bucket label, if room
      if (dx > 15) {
         ctx.textAlign = 'center'
         ctx.textBaseline = 'top'
         ctx.fillText(b, x + dx/2, 2)
      }
      x += dx
   }
   // bucket right edge
   ctx.lineWidth= 1
   ctx.strokeStyle= '#333'
   ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, 20); ctx.stroke();
}

const canvas= ref(null);

function render() {
   new_layout= calc_layout(props.rbhash, props.user_array)
   let rect= canvas.value.getBoundingClientRect()
   let scale= window.devicePixelRatio;
   canvas.value.width= rect.width * scale;
   canvas.value.height= rect.height * scale;
   let ctx= canvas.value.getContext('2d')
   ctx.reset()
   ctx.scale(scale, scale)
   render_layout(new_layout, ctx, canvas.value.width)
}
onMounted(render)
watch(() => props.rbhash, render)

defineExpose({
   render: render
})

</script>

<template>
   <canvas ref="canvas">
   </canvas>
</template>

<style scoped>
canvas { border: none; min-width: 600px; min-height: 400px; width: 100%; }
</style>