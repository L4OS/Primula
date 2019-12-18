all:
		cd lexical_parser && $(MAKE)
		cd ../syntaxparser && &(MAKE)
