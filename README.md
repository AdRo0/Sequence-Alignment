# Sequence-Alignment

Command-line utility that compares two files using the Smith–Waterman algorithm to identify local similarities.

### Usage

```bash
$ align <file1> <file2> [OPTIONS]

Options:
  -n <#>    Show alignments of <#> islands.
  -h        Hex mode.
  -ma <#>   Match score (default 1).
  -mis <#>  Mismatch score (default -1).
  -gp <#>   Gap penalty (default -2).

Example:
  align fileA.txt fileB.txt -n 5 -h
