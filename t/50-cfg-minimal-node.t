#! /usr/bin/env perl
use Test2::V0;
use FindBin;
use lib "$FindBin::RealBin/lib";
use LocalTestUtil ':all';
use JSON;
use Config;
use CodeGen::RedBlackTree::C;

my $rbgen= CodeGen::RedBlackTree::C->new(
   with_count => 0,
   with_parent => 1,
   with_data_pointer => 0,
   cmp => 'numeric',
   key_type => 'size_t:15',
);

ok( $rbgen->generate_rb_api->write(\my($h, $c)), 'Generated source code' );

{
   skip 1, 'No C compiler available' unless have_c_compiler;
   my $result= c_eval $h, $c, <<~C, <<~main;
      void dump_node(rbtree_node_t *node) {
         rbtree_node_t *left= rbtree_node_left(node);
         rbtree_node_t *right= rbtree_node_right(node);
         int color= rbtree_node_color(node);
         printf("[ ");
         if (left) dump_node(left); else printf("null");
         printf(", \\"%s\\", %d, ", color? "r" : "b", (int) node->key);
         if (right) dump_node(right); else printf("null");
         printf(" ]\\n");
      }
      C
         rbtree_tree_t tree;
         rbtree_node_t nodes[20];
         memset(nodes, 0, sizeof(nodes));
         rbtree_init_tree(&tree, 0);
         for (int i=0; i < sizeof(nodes)/sizeof(*nodes); i++) {
            nodes[i].key= i;
            rbtree_tree_insert(&tree, nodes+i);
         }
         dump_node(tree.root_sentinel.left);
      main
   my $tree= JSON->new->decode($result);
   is( $tree,
      [
         [
            [
               [ undef, b => 0, undef ],
               b => 1,
               [ undef, b => 2, undef ],
            ],
            r => 3,
            [
               [ undef, b => 4, undef ],
               b => 5,
               [ undef, b => 6, undef ],
            ],
         ],
         b => 7,
         [
            [
               [ undef, b => 8, undef ],
               b => 9,
               [ undef, b => 10, undef ],
            ],
            r => 11,
            [
               [
                  [ undef, b => 12, undef ],
                  r => 13,
                  [ undef, b => 14, undef ],
               ],
               b => 15,
               [
                  [ undef, b => 16, undef ],
                  r => 17,
                  [ undef, b => 18, [ undef, r => 19, undef ] ],
               ]
            ]
         ],
      ],
      'tree structure'
   ) or do { use DDP; main::p($tree) };
}

done_testing;