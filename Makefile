all:
		cd lexical_parser && $(MAKE)
		cd../syntaxparser && &(MAKE)

check:
		echo "Tests under construction"

distcheck:
		echo "TODO: eliminate distcheck test"