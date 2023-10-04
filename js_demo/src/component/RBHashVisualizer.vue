<script setup>
import { ref, watch, onMounted } from 'vue'

const props= defineProps({
   rbhash: Object,
   user_array: Object,
   markup: Object,
   node_size: Number
})

const canvas= ref(null);
let drag_target;
let animation_timer_id;

let anim_layout= { buckets: [], nodes: [] };
let final_layout;

/* Layout structure:

  {
   nodes: [
     { id: , width: , height: , x: , y: , 0: , 1: , red: , parent_id: , highlight: }
     ...
   ]
   buckets: [
     { idx: , x: , dx: , node_id: , higghlight: }
   ],
   node_rad:
   node_dx:
   node_dy:
   node_width_sum:
  }

*/
function calc_layout(rbhash, user_array, markup, canvas_rect) {
   let highlight= markup && markup.highlight? markup.highlight : {}
   let buckets= new Array(rbhash.n_buckets)
   let nodes= new Array(user_array.length+1)
   for (let node_id= 1; node_id <= user_array.length; node_id++)
      nodes[node_id]= {
         id: node_id,
         key: user_array[node_id-1],
         highlight: highlight['node'+node_id]
      }

   let build_subtree_layout= (node) => {
      node.width= 1;
      node.height= 1;
      for (let subtree= 0; subtree <= 1; subtree++) {
         let ref= rbhash.array[node.id * 2 + subtree]
         if (ref) {
            build_subtree_layout(node[subtree]= nodes[ref>>1])
            node[subtree].parent_id= node.id;
            if (ref & 1) node[subtree].red= 1
            node.width += node[subtree].width
            node.height= Math.max(node.height, 1 + node[subtree].height);
         }
      }
   }

   // For each filled bucket, count number of nodes in that tree
   let width= 0;
   let widths= [];
   for (let b= 0; b < buckets.length; b++) {
      buckets[b]= { idx: b, highlight: highlight['bucket'+b] }
      let node_id= rbhash.array[rbhash.table_ofs + b]
      if (node_id) {
         buckets[b].node_id= node_id
         let node= buckets[b].node= nodes[node_id]
         build_subtree_layout(node)
         width += node.width
         widths.push(node.width)
      }
   }

   // Now figure out the pixel coordinates of everything
   let node_diam= parseFloat(props.node_size || 20)
   let node_rad= node_diam / 2
   let node_dx= node_diam + 2
   let node_dy= node_diam + 2
   
   // make a list of the widest buckets
   widths= widths.sort()
   let widest_sum= widths.reduce((ret, cur) => ret + cur, 0);
   let standard_bucket_count= buckets.length - widths.length
   let standard_bucket_dx;
   // Does the minimal width of all the nodes occupy all the space?
   if (widest_sum >= canvas_rect.width) {
      standard_bucket_dx= 3
   } else {
      // for each small node, can we make all buckets at least this large without running out of room?
      while (widths.length && (widest_sum - widths[0]) + (widths[0] * standard_bucket_count) <= canvas_rect.width/node_dx) {
         //console.log(widest_sum, widths.concat([]), standard_bucket_count)
         // convert least-wide to a standard bucket width
         widest_sum -= widths[0]
         widths.shift();
         standard_bucket_count++
      }
      // divide remaiing space among the standard buckets
      standard_bucket_dx= (canvas_rect.width - widest_sum*node_dx) / standard_bucket_count;
      //console.log(widest_sum, widths.concat([]), standard_bucket_count, canvas_rect.width, standard_bucket_dx)
   }
   
   // Now that bucket widths are known, give everything an X and Y coordinate
   let calc_subtree_coordinates= function(node, x, y) {
      node.x= x + node_dx * (node[0]? node[0].width:0) + node_dx/2;
      node.y= y + node_dy/2;
      if (node[0]) calc_subtree_coordinates(node[0], x, y + node_dy);
      if (node[1]) calc_subtree_coordinates(node[1], node.x + node_dx/2, y + node_dy);
   }
   
   // mark the boundaries of each bucket, and then position the tree within
   let x= 0
   for (let b= 0; b < buckets.length; b++) {
      let dx;
      // bucket tree
      let node= buckets[b].node
      if (node) {
         let tree_dx= node_dx * node.width
         dx= Math.max(tree_dx, standard_bucket_dx)
         calc_subtree_coordinates(node, x + (dx - tree_dx)/2, 15)
      } else {
         dx= standard_bucket_dx
      }
      buckets[b].dx= dx
      x += dx
   }
   // during insert, the hints include the location where the new node is being
   // compared.  The new node is not in the tree yet, so set (x,y) relative to
   // comparison target
   if (markup && markup.insert_at) {
      let ins_node= nodes[markup.insert_at[0]];
      let target_node= nodes[markup.insert_at[1]];
      if (!('x' in ins_node)) {
         ins_node.x= target_node.x + node_dx * .8
         ins_node.y= target_node.y - node_dx * .8
         target_node.ins_relative= ins_node
      }
   }

   return { nodes, buckets, canvas_rect, node_rad, node_dx, node_dy }
}

function slide(value, goal) {
   value += (goal - value)*.1;
   if (Math.abs(goal-value) < .1)
      value= goal;
   return value;
}
function animate_layout(layout, goal) {
   // Remove nodes that no longer exist
   while (layout.nodes.length > goal.nodes.length)
      layout.nodes.pop()
   // Same for buckets
   while (layout.buckets.length > goal.buckets.length)
      layout.buckets.pop()
   while (layout.buckets.length < goal.buckets.length)
      layout.buckets.push({ idx: layout.buckets.length, x: goal.canvas_rect.width, dx: 0 })
   layout.canvas_rect= goal.canvas_rect
   layout.node_rad= goal.node_rad
   layout.node_dx= goal.node_dx
   layout.node_dy= goal.node_dy

   let pending_motion= false;

   // For each bucket
   let x= 0;
   for (let b= 0; b < layout.buckets.length; b++) {
      let bucket= layout.buckets[b], goal_bucket= goal.buckets[b]
      // Wait until we encounter a missing node before creating it,
      // so we can animate it entering from the current bucket's (x,y)
      let add_missing_node= function(node_id) {
         while (layout.nodes.length <= node_id)
            layout.nodes.push(null);
         return layout.nodes[node_id]= {
            id: node_id, key: goal.nodes[node_id].key,
            x: bucket.x+bucket.dx/2, y: 0
         }
      }
      // subtree x,y are adjusted according to their offset from parent, so that
      // moving the parent moves the whole subtree at the same speed
      let adjust_subtrees= function(node, goal_node) {
         node.red= goal_node.red
         node.highlight= goal_node.highlight
         if (node.x != goal_node.x || node.y != goal_node.y) {
            //console.log('node '+node.id+' change:', node, goal_node)
            pending_motion= true
         }
         for (let subtree= 0; subtree < 2; subtree++) {
            if (goal_node[subtree]) {
               node[subtree]= layout.nodes[goal_node[subtree].id];
               if (!node[subtree])
                  node[subtree]= add_missing_node(goal_node[subtree].id)
               let child= node[subtree], goal_child= goal_node[subtree]
               // If parent_id has changed, reset the x_ofs and y_ofs based on current absolute position
               if (child.parent_id != goal_child.parent_id) {
                  child.parent_id= goal_child.parent_id
                  child.x_ofs= child.x - node.x
                  child.y_ofs= child.y - node.y
               }
               // If node being dragged, don't move it
               // Else continue animation based on existing x_ofs and y_ofs
               if (child.id != drag_target) {
                  // Calculate desired displacement from parent
                  let goal_x_ofs= goal_child.x - goal_node.x
                  let goal_y_ofs= goal_child.y - goal_node.y
                  child.x= node.x + (child.x_ofs= slide(child.x_ofs, goal_x_ofs))
                  child.y= node.y + (child.y_ofs= slide(child.y_ofs, goal_y_ofs))
                  child.goal_x_ofs= goal_x_ofs
               }
               adjust_subtrees(node[subtree], goal_node[subtree]);
            } else {
               delete node[subtree];
            }
         }
         if (goal_node.ins_relative) {
            node.ins_relative= layout.nodes[goal_node.ins_relative.id] || add_missing_node(goal_node.ins_relative.id)
            let child= node.ins_relative
            child.x= slide(child.x, goal_node.ins_relative.x)
            child.y= slide(child.y, goal_node.ins_relative.y)
            node.ins_relative.highlight= 2
            if (child.x != goal_node.ins_relative.x || child.y != goal_node.ins_relative.y)
               pending_motion= true
         }
         else delete node.ins_relative
      }
      // stretch it toward its correct dx
      bucket.dx= slide(bucket.dx, goal_bucket.dx)
      // but stack them from the left regardless of where they are
      bucket.x= x
      x += bucket.dx
      if (bucket.dx != goal_bucket.dx) {
         //console.log('bucket '+b+' change:', bucket, goal_bucket)
         pending_motion= true
      }
      // If tree rooted here, move its root node
      if (goal_bucket.node_id) {
         bucket.node_id= goal_bucket.node_id
         bucket.node= layout.nodes[bucket.node_id]
         if (!bucket.node)
            bucket.node= add_missing_node(bucket.node_id, bucket)
         if (bucket.node_id != drag_target) {
            bucket.node.x= slide(bucket.node.x, goal_bucket.node.x)
            bucket.node.y= slide(bucket.node.y, goal_bucket.node.y)
         }
         delete bucket.node.parent_id
         adjust_subtrees(bucket.node, goal_bucket.node)
      } else {
         delete bucket.node_id
         delete bucket.node
      }
   }
   // do coordinate differences remain?
   return pending_motion;
}

function render_layout(layout, ctx) {
   // Render the bucket markers
   let x= 0
   for (let b= 0; b < layout.buckets.length; b++) {
      let dx= layout.buckets[b].dx
      // bucket left edge
      ctx.lineWidth= 1
      ctx.strokeStyle= '#333'
      ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, 20); ctx.stroke();
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
   // Now render links between nodes
   for (let i= 1; i < layout.nodes.length; i++) {
      let node= layout.nodes[i]
      if (node.parent_id) {
         // don't render unless it's within 50% of the distance it's supposed to be
         let parent= layout.nodes[node.parent_id];
         let parent_dx= node.x - parent.x;
         if (1 || !node.goal_parent_dx || (parent_dx / node.goal_parent_dx > .5)) {
            ctx.lineWidth= 1;
            ctx.strokeStyle= node.red? "red" : "black";
            ctx.beginPath(); ctx.moveTo(node.x, node.y); ctx.lineTo(parent.x, parent.y); ctx.stroke();
         }
      }
   }
   // Now render the nodes, overtop the ends of the links
   for (let i= 1; i < layout.nodes.length; i++) {
      let node= layout.nodes[i]
      if (node.key != null) {
         if (node.highlight || node.id == props.markup.selected_node) {
            ctx.lineWidth= 4
            ctx.strokeStyle= node.highlight == 2? '#BFB' : '#DD0'
            ctx.beginPath()
            ctx.arc(node.x, node.y, layout.node_rad+1, 0, 2*Math.PI)
            ctx.stroke()
         }
         ctx.lineWidth= 1
         ctx.strokeStyle= node.red? "red" : "black"
         ctx.beginPath()
         ctx.arc(node.x, node.y, layout.node_rad, 0, 2*Math.PI)
         ctx.fillStyle= node.highlight == 1? (node.red? '#FD7' : '#EE0')
            : node.highlight == 2? (node.red? '#DF7' : '#BFB')
            : (node.red? '#FDD' : '#EEE')
         ctx.fill()
         ctx.stroke()
         ctx.fillStyle= node.red? 'red':'black'
         ctx.textAlign= 'center'
         ctx.textBaseline= 'middle'
         ctx.fillText(node.id, node.x, node.y)
         // render the key, downward
         ctx.save()
         ctx.translate(node.x, node.y+layout.node_rad+3)
         ctx.rotate(Math.PI/2)
         ctx.textAlign= 'left'
         ctx.fillText(node.key, 0,0);
         ctx.restore()
      }
   }
}

function animate(enable) {
   if (enable) {
      if (!animation_timer_id) {
         animation_timer_id= setInterval(rerender, 1000/60);
         //console.log('animate=true')
      }
   } else {
      if (animation_timer_id) {
         clearInterval(animation_timer_id)
         animation_timer_id= null;
         //console.log('animate=false')
      }
   }
}

function render() {
   //console.log('render', !!canvas.value);
   if (!canvas.value) return
   let rect= canvas.value.getBoundingClientRect()
   let scale= window.devicePixelRatio || 1;
   canvas.value.width= rect.width * scale;
   canvas.value.height= rect.height * scale;
   final_layout= calc_layout(props.rbhash, props.user_array, props.markup, rect)
   rerender()
}
function rerender() {
   //console.log('rerender', !!canvas.value, JSON.stringify(props.markup.insert_at), anim_layout.nodes);
   if (!canvas.value) return
   let scale= window.devicePixelRatio || 1;
   let ctx= canvas.value.getContext('2d')
   ctx.reset()
   ctx.scale(scale, scale)
   animate(animate_layout(anim_layout, final_layout))
   render_layout(anim_layout, ctx)
}

// Render when canvas is first created, and every time props.rbhash changes
onMounted(render)
watch(() => props.rbhash, render)
watch(props.markup, (m) => { m.insert_at? render() : rerender() })

defineExpose({ render: render })
const emit= defineEmits([ 'nodeClick' ])

function distance(dx, dy) {
   return Math.sqrt(dx*dx + dy*dy)
}
function canvas_mousedown(ev) {
   let rect= ev.target.getBoundingClientRect()
   let x= ev.clientX - rect.x;
   let y= ev.clientY - rect.y;
   // Did this click land on a node?
   for (let i=1; i < anim_layout.nodes.length; i++) {
      let node= anim_layout.nodes[i];
      if (distance(node.x - x, node.y - y) <= anim_layout.node_rad) {
         drag_target= node.id
         // also emit an event
         emit('nodeClick', { node_id: node.id })
         break
      }
   }
}
function canvas_mouseup(ev) {
   // capture the new distance from parent for node being released
   if (drag_target) {
      let node= anim_layout.nodes[drag_target];
      if (node && node.parent_id) {
         let parent= anim_layout.nodes[node.parent_id]
         node.x_ofs= node.x - parent.x
         node.y_ofs= node.y - parent.y
      }
      animate(true)
   }
   drag_target= 0
}
function canvas_mousemove(ev) {
   let rect= ev.target.getBoundingClientRect()
   let x= ev.clientX - rect.x;
   let y= ev.clientY - rect.y;
   if (drag_target && drag_target < anim_layout.nodes.length) {
      anim_layout.nodes[drag_target].x= x;
      anim_layout.nodes[drag_target].y= y;
      animate(true)
   }
}

</script>

<template>
   <canvas ref="canvas"
      @mousedown="canvas_mousedown" @mouseup="canvas_mouseup" @mousemove="canvas_mousemove"
   >
   </canvas>
</template>

<style scoped>
canvas { border: none; min-width: 600px; height: 300px; width: 100%; }
</style>