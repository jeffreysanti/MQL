# MQL Matrix Query Language

A Simple and extendable stack based programming language
designed for querying and analysis of datasets in a database.

MQL aims to provide an intutive way of interacting with a set
of data without comprimising flexibility. Despite being designed
for interactive use, it is general purpose in the sense that any
computational problem can be solved using it.

Features include a Forth-inspired syntax which allows substantial
customization of the syntax and semantics of the language. It won't
stop you from overriding a built in operator if you don't like 
the way one works for instance. There is little built into the
language that is really fundamental to its functionality, while
much is added merely for convienence and performance.

MQL uses four fundamental data types: strings, integers, decimals,
and vectors. Each object can have arbitrarily many operators
attached to it (in addition to global operators). The idea is
that a vector can be accessed with an index, but also by
custom operators. When used with a database, MQL will automatically
generate relevant operators to match a table schema. 

For instance,
a table `wdist` with a word and count field will generate an operator
called `wdist` which returns a vector of row vectors with the relevant
access operators, such that you can do the following:
```
wdist word # Returns a vector of all words
wdist count # Returns a vector of all wordcounts

# Builds an vector of word-percentusage pairs
:wdist% {
	wdist count sum ! __total . # Saves total of all words
	wdist for dup [word, count __total / 100 *] 
			:percent 2 get ; # for each row we can get percent component
			:count nop ; # count not relevant
		push orf
	:percent transpose 2 get ; # get list of all percents
	:count nop ; # not relevant
} 
``` 

## Sample Code (Working Right Now)
```
:fib { dup dup 
	0 = if { . . 0 }
	else { 1 = if { . 1 }
	else { dup 1 - fib swap 2 - fib + } }
}

:count { 0 ! __countx .
	for 1 __countx + ! __countx . continue orf
	. __countx
}


: upto {
	dup
	0 = if 0
	else { dup 1 - upto + }
}

```



