#! /usr/bin/env perl
use v5.36;
use Mojolicious::Lite;
use CodeGen::Cpppp;

my $cpppp= CodeGen::Cpppp->new(include_path => [app->home]);
$cpppp->require_template('rbhash.cpppp');

get '/' => sub($c) { $c->reply->static('index.html') };

app->start;
