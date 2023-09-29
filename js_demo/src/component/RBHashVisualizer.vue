<script setup>
import { ref, onMounted } from 'vue'

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
   let empty_bucket_count= layout.n_buckets - Object.keys(layout.table).length
   let empty_bucket_dx= 3
   let node_dx= layout.node_dx
   // widen empty buckets to use space not used by nodes
   if (layout.node_width_sum * props.node_size + empty_bucket_count * empty_bucket_dx < canvas_width)
      empty_bucket_dx= (canvas_width - layout.node_width_sum * node_dx) / empty_bucket_count;
   // render each filled bucket, first rendering the empty buckets to the left.
   let x= 0
   let last_b= -1
   Object.keys(layout.table).sort((a,b) => parseInt(a) < parseInt(b)).forEach((b) => {
      // render buckets last_b+1 .. b-1
      ctx.lineWidth= 1
      ctx.strokeStyle= '#333'
      for (let i= last_b+1; i < b; i++) {
         x += empty_bucket_dx
         ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, 10); ctx.stroke();
      }
      // render tree
      render_subtree(layout, layout.table[b].node, x, 10, ctx);
   });
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

defineExpose({
   render: render
})

</script>

<template>
   <canvas ref="canvas">
   </canvas>
</template>

<style scoped>
canvas { border: 1px solid black; min-width: 600px; min-height: 400px; }
</style>