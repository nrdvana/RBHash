#! /usr/bin/env perl
use Test2::V0;
use FindBin;
use lib "$FindBin::RealBin/lib";
use LocalTestUtil;
use Config;
use Lang::Generate::RedBlackTree::C;

my @tests= (
   [ 'default', { size_size => $Config{sizesize} }, <<~C ],
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
   C
   [ 'at_offset', { size_size => $Config{sizesize}, node_offset => 'node' }, <<~C ],
   struct rbtree_node;
   typedef struct rbtree_node rbtree_node_t;
   struct rbtree_node {
     rbtree_node_t* left;
     rbtree_node_t* right;
     rbtree_node_t* parent;
     size_t         color : 1;
     size_t         count : @{[ $Config{sizesize} *8 - 1 ]};
   };
   C
   [ 'no_count_cust_name_int_key', {
         size_size => $Config{sizesize},
         node_type => 'MyNode_t',
         key_type => 'int',
         cmp => 'numeric',
         with_count => 0
      }, <<~C ],
   struct rbtree_node;
   typedef struct rbtree_node MyNode_t;
   struct rbtree_node {
     MyNode_t* left;
     MyNode_t* right;
     MyNode_t* parent;
     void*     data;
     size_t    color : 1;
     int       key;
   };
   C
);

for (@tests) {
   my ($name, $opts, $expected)= @$_;
   my $rbgen= Lang::Generate::RedBlackTree::C->new($opts);
   is( $rbgen->_node_type_decl, $expected, $name );
}

done_testing;