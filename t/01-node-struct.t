#! /usr/bin/env perl
use Test2::V0;
use FindBin;
use lib "$FindBin::RealBin/lib";
use LocalTestUtil;
use Config;
use CodeGen::RedBlackTree::C;

my @tests= (
   [ 'default', {}, <<~C ],
   struct rbtree_node;
   typedef struct rbtree_node rbtree_node_t;
   struct rbtree_node {
      rbtree_node_t* left;
      rbtree_node_t* right;
      rbtree_node_t* parent;
      void*          data;
      size_t         color : 1;
      size_t         count : @{[ $Config{sizesize} *8 - 1 ]};
   };
   typedef int (*rbtree_compare_fp)(void *context, void *key_a, void *key_b);
   struct rbtree_tree {
      rbtree_node_t root_sentinel;
      rbtree_node_t leaf_sentinel;
      ptrdiff_t data_to_key_ofs;
      rbtree_compare_fp cmp;
      void* cmp_context;
   };
   typedef struct rbtree_tree rbtree_tree_t;
   C
   [ 'minimal', {
         with_parent => 0,
         with_count => 0,
         with_data_pointer => 0,
         with_packed_color => 1,
      }, <<~C ],
   struct rbtree_node;
   typedef struct rbtree_node rbtree_node_t;
   struct rbtree_node {
      rbtree_node_t* left;
      uintptr_t right;
   };
   typedef int (*rbtree_compare_fp)(void *context, void *key_a, void *key_b);
   struct rbtree_tree {
      rbtree_node_t root_sentinel;
      rbtree_node_t leaf_sentinel;
      ptrdiff_t node_to_data_ofs;
      ptrdiff_t data_to_key_ofs;
      rbtree_compare_fp cmp;
      void* cmp_context;
   };
   typedef struct rbtree_tree rbtree_tree_t;
   C
   [ 'no_count_cust_int_key', {
         cmp => 'numeric',
         key_type => 'size_t:15',
         node_type => '*MyNode_p',
         node_struct => 'MyNode_s',
         with_count => 0,
         with_data_pointer => 0,
         node_fields => { key => "sort_order" },
      }, <<~C ],
   struct MyNode_s;
   typedef struct MyNode_s *MyNode_p;
   struct MyNode_s {
      MyNode_p left;
      MyNode_p right;
      MyNode_p parent;
      size_t   color : 1;
      size_t   sort_order:15;
   };
   struct rbtree_tree {
      struct MyNode_s root_sentinel;
      struct MyNode_s leaf_sentinel;
      ptrdiff_t node_to_data_ofs;
   };
   typedef struct rbtree_tree rbtree_tree_t;
   C
);

for (@tests) {
   my ($name, $opts, $expected)= @$_;
   my $rbgen= CodeGen::RedBlackTree::C->new($opts);
   $rbgen->generate_node_struct->write(\my ($h, $c));
   $h =~ s: *//.*::g; # strip out comments so that they don't affect unit tests
   is( $h, $expected, $name );
}

done_testing;