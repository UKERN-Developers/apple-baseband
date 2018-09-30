use strict;

sub printUsage;
sub writeRegister;
sub writeRegisterMasked;


# CLKCON5
my $kDefaultRegister = 0x3C500014;

# Upper half, CK24O in the case of CLKCON5
my $kDefaultShift = 16;

# PLL1
my $kDefaultPLL = 1;

# divider = 8
my $kDefaultDivider = 3;

# mem access
my $kMemAccess = "RegisterAccess";

# =============================

my $argc = $#ARGV + 1;

my $reg 		= $kDefaultRegister;
my $shift 		= $kDefaultShift;
my $PLL			= $kDefaultPLL;
my $div			= $kDefaultDivider;

for( my $i = 0; $i < $argc; $i+=2 )
{
	my $c = @ARGV[$i];
	my $next = @ARGV[$i+1];
	if ( $c eq "-r" )
		{
			$reg = hex( $next );
		}
	elsif ( $c eq "-s" )
		{
			$shift = int( $next );
			if ( $shift != 0 && $shift != 16 )
			{
				print "Shift $shift must be either 0 or 16\n";
				printUsage();
				exit();
			}
		}

	elsif ( $c eq "-p" )
		{
			$PLL = int( $next );

			if ( $PLL < 0 || $PLL > 3 ) 
			{
				print "PLL $PLL must be between 0 and 3 inclusive.\n";
				printUsage();
				exit();
			}
		}

	elsif ( $c eq "-d" )
		{
			$div = int( $next );

			if ( $div < 0 || $div > 3 )
			{
				print "Divider $div must be between 0 and 3 inclusive.\n";
				printUsage();
				exit();
			}
		}

	elsif ( $c eq "-h" )
		{
			printUsage();
			exit();
		}
	else
		{
			print "Unrecognized option $c\n";
			printUsage();
			exit();
		}
	
}

printf( "Using register 0x%08X, shift %d, PLL %d, divider %d(%d)\n", $reg, $shift, $PLL, (1<<$div), $div );

my $cmd = "";
my $mask = 0xFFFF << $shift; 

my $val = 
	( 0 << ( 15 + $shift ) ) 		# 0 means enable
	| ( 1 << ( 14 + $shift ) ) 		# use mux output clock
	| ( $PLL << ( 12 + $shift ) ) 	# PLL
	| ( 1 << ( 8 + $shift ) )		# prescaler on
	| ( $div << ( 0 + $shift ) )	# divider
	;

writeRegisterMasked( $reg, $val, $mask );


sub printUsage
{
	print "Options:\n";
	printf( "-r reister     : Set the register, default is 0x%08X\n", $kDefaultRegister );
	printf( "-s shift       : Set the shift into the register, default is %s\n", $kDefaultShift );
	printf( "-p PLL         : Set the PLL to use as the source, default is %s\n", $kDefaultPLL );
	printf( "-d divider     : Set the divider, default is %s\n", $kDefaultDivider );
	printf( "-h             : Print help\n", $kDefaultDivider );
}

sub writeRegister
{
	my $r = shift;
	my $v = shift;

	my $cmd = sprintf( "%s 0x%08X 0x%08X", $kMemAccess, $r, $v );
	printf( "Using command %s\n", $cmd );
	system( $cmd );
}

sub writeRegisterMasked
{
	my $r = shift;
	my $v = shift;
	my $m = shift;

	my $cmd = sprintf( "%s 0x%08X 0x%08X 0x%08X", $kMemAccess, $r, $v, $m );
	printf( "Using command %s\n", $cmd );
	system( $cmd );
}

