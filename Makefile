all: 
		make -C lexic_parser 
		make -C syntax_parser 

check:
		make -C check

distcheck:
		echo "TODO: eliminate distcheck test"
