EXEC = dokumentace

all: $(EXEC)

$(EXEC): 
	pdflatex $(EXEC)
	pdflatex $(EXEC)
	
clean: 
	rm $(EXEC).log
	rm $(EXEC).aux
	rm $(EXEC).out
