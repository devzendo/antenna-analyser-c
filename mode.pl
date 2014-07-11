#!/opt/local/bin/perl
use warnings;
use strict;

my @values = ();
my %occs = ();
my $total = 0;
my $count = 0;
my $min = 99999;
my $max = 0;

die "No data file as arg" unless ($ARGV[0]);

my $file = $ARGV[0];
local *IN;
open(IN, "<$file") || die "Can't open $file: $!\n";

while (<IN>) {
  chomp;
  if (/(\d+) (\d+)/) {
    push @values, $2;
    $occs{$2} ||= 0;
    $occs{$2}++;
    $count++;
    $total += $2;
    if ($2 > $max) {
      $max = $2;
    }
    if ($2 < $min) {
      $min = $2;
    }
  }
}
close IN;

my $largestocc = 0;
my $largestoccval = 0;
my $occ;
foreach $occ (keys(%occs)) {
  if ($occs{$occ} > $largestoccval) {
    $largestoccval = $occs{$occ};
    $largestocc = $occ;
  }
}
print "Mean: " . ($total / $count) . "\n";
print "Mode: $largestocc ($largestoccval occurrences)\n";
print "Min: $min Max $max\n";
print "Between min and max: " . ($min + (($max - $min) / 2)) . "\n";
my @sorted = sort {$occs{$a} <=> $occs{$b}} (keys(%occs));
foreach (@sorted) {
  print "$_ has $occs{$_} occurrences\n";
}

my $gnuplotcmd = "/tmp/gnuplot.cmd";
local *FILE;
open(FILE, ">$gnuplotcmd") || die "Can't open $gnuplotcmd: $!\n";
print FILE <<EOF;
set term aqua size 600,400
set xtics scale 2,1
set mxtics 5
set linetype 1 lw 1 lc rgb "blue" pointtype 0
set xlabel 'samples'
set ylabel 'forward'
plot '$file' smooth bezier, '$file' with points
EOF
close FILE;
system("gnuplot $gnuplotcmd");


