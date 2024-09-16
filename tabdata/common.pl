#!/usr/bin/env perl

$0 =~ s/^.*\/([^\/]+)$/$1/;

# Note: review escape_tabdata subroutine before you think about changing $FS and $RS fundamental variables.
# Note: review "split $FS" calls if you ever change $FS to a single space char. "split ' '" is handled specially by Perl.
$FS = "\t";
$RS = $/ = "\n";
@Header = ();
%Header = ();

no if ($] >= 5.018), 'warnings' => 'experimental::smartmatch';
use Getopt::Long qw/:config no_ignore_case bundling pass_through require_order no_getopt_compat no_auto_abbrev/;
use Pod::Usage;

binmode STDIN,  ':utf8';
binmode STDOUT, ':utf8';


if(not $TabdataCommonSkipGetopt)
{
	GetOptions(
		'help|?' => sub{ pod2usage(-exitval=>0, -verbose=>99); },
		%OptionDefs,
	) or pod2usage(-exitval=>2, -verbose=>99);
	
	if('--' ~~ @ARGV and $ARGV[0] ne '--')
	{
		# at least 1 unknown option remained in @ARGV
		pod2usage(-exitval=>2, -verbose=>99, -msg=>"$0: unknown parameter: $ARGV[0]");
	}
	
	# when pass_through option is set:
	# Note that the options terminator (default "--"), if present, will also be passed through in @ARGV.
	shift @ARGV if $ARGV[0] eq '--';
}


sub process_header
{
	$HeaderLine = $_[0];
	chomp $HeaderLine;
	@Header = split $FS, $HeaderLine;
	%Header = ();
	for my $idx (0..$#Header)
	{
		$Header{$Header[$idx]} = $idx;
	}
}

sub sys_read_line
{
	# read data from STDIN until $RS (usually newline) or EOF, whichever comes first.
	# and return with data.
	# $RS is consumed by not returned.
	# useful if the rest of STDIN will not be processed by the current process.
	
	my $line = '';
	my $c;
	$line .= $c while sysread(STDIN, $c, 1) and $c ne $RS;
	return $line;
}

sub read_record
{
	my $fd = shift;
	my $line = <$fd>;
	chomp $line;
	my @record = split $FS, $line;
	return @record;
}

sub escape_tabdata
{
	my $arbitrary_data = shift;
	# Note, may be wrong if $FS or $RS are changed.
	my $tabdata = $arbitrary_data =~ s/[\t\n\r\e\\]/'\\'.{"\t"=>'t', "\n"=>'n', "\r"=>'r', "\e"=>'e', "\\"=>'\\'}->{$&}/ger;
	return $tabdata;
}

sub unescape_tabdata
{
	my $tabdata = shift;
	my $raw = $tabdata =~ s/\\[tnre\\]/eval "\"$&\""/ger;
	return $raw;
}

sub kvpair_escape
{
	my $s = shift;
	if($s =~ /[""'' ]/)
	{
		$s =~ s/[\x00-\x1F\x7F""\\]/sprintf '\\x%02X', ord $&/eg;
		$s = "\"$s\"";
	}
	return $s;
}

sub kvpair_unescape
{
	my $s = shift;
	if($s =~ /^(?'quote'[""''])(?'value'.*?)\g{quote}$/)
	{
		$s = $+{'value'} =~ s/\\x([[:xdigit:]]{2})/chr hex $1/egir;
	}
	return $s;
}

1;
