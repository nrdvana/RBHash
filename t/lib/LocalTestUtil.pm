package LocalTestUtil;
use v5.26;
use Exporter 'import';
use File::Temp;
use Carp;
use Capture::Tiny 'capture';
our @EXPORT_OK= qw( have_c_compiler c_eval sizeof_ptr sizeof_size );
our %EXPORT_TAGS= ( all => \@EXPORT_OK );
our $DEBUG= $ENV{DEBUG} // 0;
our $cc;
sub have_c_compiler {
   $cc ||= _detect_compiler();
}
sub _detect_compiler {
   chomp($cc= `which cc`);
   return $cc if c_eval(q{printf("1")}) eq "1";
   return 0;
}
sub c_compile {
   my $src= shift;
   $cc ||= _detect_compiler || croak "No C compiler detected";
   my $c_file= _new_c_file($src);
   my $exe_file= ($c_file =~ s/\.c$/\.exe/r); # use .exe since windows requires it and linux doesn't care
   return undef if system($cc, '-o', $exe_file, $c_file) != 0;
   return $exe_file;
}

sub _new_c_file {
   state $cc_dir = File::Temp->newdir($DEBUG? (CLEANUP => 0) : ());
   my $content= shift;
   my $tmp= File::Temp->new(DIR => $cc_dir, SUFFIX => '.c', ($DEBUG? (UNLINK => 0) : ()));
   $tmp->print($content) or die "write: $!";
   close $tmp or die "close: $!";
   $tmp;
}

$File::Temp::Keep_All= 1;

sub c_eval {
   my (@c_src)= @_;
   # wrap final arg with main()
   $c_src[-1] =~ s/^\s+//;
   $c_src[-1] =~ s/\s*;\s*$//;
   $c_src[-1]= "int main() {\n  $c_src[-1];\n  return 0;\n}";
   unshift @c_src, <<~C;
      #include <stdlib.h>
      #include <stdio.h>
      #include <string.h>
      #include <stddef.h>
      #include <stdbool.h>
      #include <assert.h>
      C
   my $src= join("\n", @c_src, '');
   my ($stdout, $stderr, $exe_file)= capture { c_compile($src); };
   defined $exe_file
      or croak "\n$src\n$stdout\n$stderr\nFailed to compile: $?";
   ($stdout, $stderr, my $exit)= capture { system($exe_file) };
   $exit == 0
      or croak "\n$src\n$stdout\n$stderr\nEval failed: $?";
   say $stderr;
   chomp $stdout;
   return $stdout;
}

sub sizeof {
   state %sizes= do {
      split /\s+/, c_eval(q{
         printf("size_t %ld void* %ld", sizeof(size_t), sizeof(void*));
      });
   };
   \%sizes;
}

sub sizeof_ptr { sizeof()->{'void*'} }
sub sizeof_size { sizeof()->{size_t} }

1;
