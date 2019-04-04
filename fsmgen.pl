#!/usr/bin/perl -w 

die "Usage: $0 <fsmfile>\n" if (scalar(@ARGV) < 1);

open(FP,"<" . $ARGV[0]) or die "could not open file: $!\n";

$col_read = 0;
my $name;
while (<FP>) {
    next if m/^#/;
    next if m/^(\s*)$/;
    if (m/HDLR_RET\s+:=\s+(.*)/) {
        $state_ret = $1; next;
    } elsif (m/HDLR_ARGS\s+:=\s+(.*)/) {
        $state_args = $1; next;
    } elsif (m/FSM_NAME\s+:=\s+(.*)/) {
        $name = $1; next;
    } 
    if(!$col_read) {
        @cols = split /\s+/;
        $col_read = 1;
        $n = @cols - 1;
        push @tbl, "typedef $state_ret (*transition_fp) ($state_args);\n";
        push @tbl, "transition_fp const $name"."_fsm_tbl[][$n] = { \n";
        next;
    }
    @rows = split /\s+/;
    for ($i = 1;$i < scalar(@rows); $i++) {
        $fphash{$rows[$i]} = 1 if ($rows[$i] ne "NULL");
        push @tbl, "[". $rows[0]."][" .$cols[$i]. "] = ". $rows[$i]. ",\n";
    }
}
push @tbl,  "};\n";

open(TFP, ">" . $name. "_fsmtbl.h") or die "could not open file: $!\n";
@dt = `date`;
print TFP "/* Generated file, do not edit. ", @dt, "*/\n";
for $a (keys  %fphash) {
    print TFP "$state_ret ",$a, "($state_args);\n"; 
}
for $a(@tbl) {
 print TFP $a;
}
print "Generated $name", "_fsmtbl.h\n";
close(TFP);
close(FP);
