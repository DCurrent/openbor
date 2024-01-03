use strict;
use warnings;
use File::Slurp;
use File::Basename;

my $inp = $ARGV[0] or die "bin2h by anallyst - converts binaries into C headers.\nneed filename as first parameter\n";
my $f = read_file $inp or die "could not read file $inp ! $!\n";
my $var = lc(basename($inp));
$var =~ s/\./_/g;
my @a = split //, $f;
my $x = 1;
my $washex;
print "#ifndef _" . uc($var) . "_H_\n#define _" . uc($var) . "_H_\n\n";
print "static const struct {\n\tsize_t size;\n\tunsigned char data [" . scalar(@a) . "];\n} " . $var . " = {\n\t" . scalar(@a) . ",\n\"";
my $lbdelay = 0;
my $linechars = 1;
sub printable {
	my $p = shift;
	return $p =~ /^([\w \.\:;\,\-_#\+\*\$\@=\[\]\(\)\{\}\/\&\%\!<>])/;
}

sub dolb {
	print "\"\n\"";
	$linechars = 1;
	$lbdelay = 0;
}

for (@a) {
	if($linechars >= 100) {
		if($washex || !printable($_)) {
			dolb;
		} else {
			$lbdelay++;
		}		
	}
	if (($lbdelay && !printable($_)) || $lbdelay > 20) {
		dolb;
	}
	if (printable($_) && (!$washex || ($x < scalar(@a) && printable($a[$x])))) { #we only print chars directly when at least 2 of em are in a row, to save space
		if ($washex && $linechars != 1 && $x != 1) {
				print "\"\""; $linechars+=2;
		}
		$washex = 0;
		print $_; 
		$linechars++;
	} else {
		if (!$washex && $linechars != 1 && $x != 1) {
				print "\"\"";
				$linechars += 2;
		}
		$washex = 1;
		if (ord($_) < 64) {
			#print octal to save space
			my $oct = sprintf("\\%o", ord($_));
			print $oct;
			$linechars += length($oct);
		} else {	
			printf "\\x%02X", ord($_);
			$linechars += 4;
		}
	}
	$x++;
}
print "\"\n" if $linechars > 1;
print "};\n\n#endif\n";

