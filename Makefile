#---------------------------------
# INITIALISATION DES VARIABLES 
#---------------------------------
#Nom Programme
NAME_MAIN = main
NAME_GROUPES = groupes
NAME_PILOTES = pilotes

# Indiquer le compilateur
CXX= gcc

# Les chemins ou se trouvent les fichiers a inclure
INCLUDE= -I/usr/include

# Options de compilation.
CXXFLAGS=  $(INCLUDE) -c -g -std=gnu99 

# Les chemins ou se trouvent les librairies
LIBRARY_PATH= -L/usr/lib -lpthread

# Options pour le linker.
LDFLAGS= $(LIBRARY_PATH) -o

# Les librairies avec lesquelle on va effectueller l'edition de liens
LIBS=  

# Les fichiers sources de l'application
MAIN = main.c fonctions.c
GROUPES = groupes.c fonctions.c
PILOTES = pilotes.c

#-----------
# LES CIBLES
#-----------
all : $(NAME_MAIN) $(NAME_PILOTES) $(NAME_GROUPES)

$(NAME_MAIN) :  $(MAIN:.c=.o)
	$(CXX) $(LDFLAGS) $(NAME_MAIN) $(MAIN:.c=.o) $(LIBS)

$(NAME_PILOTES) :  $(PILOTES:.c=.o)
	$(CXX) $(LDFLAGS) $(NAME_PILOTES) $(PILOTES:.c=.o) $(LIBS)
	
$(NAME_GROUPES) :  $(GROUPES:.c=.o)
	$(CXX) $(LDFLAGS) $(NAME_GROUPES) $(GROUPES:.c=.o) $(LIBS)


.PHONY : clean

clean:
	/bin/rm $(MAIN:.c=.o) $(NAME_MAIN)
	/bin/rm $(GROUPES:.c=.o) $(NAME_GROUPES)
	/bin/rm $(PILOTES:.c=.o) $(NAME_PILOTES)

#-----------------------------------------------------------------------------
# LES REGLES DE DEPENDANCE. Certaines sont implicites mais je recommande d'en 
# mettre une par fichier source. 
#-----------------------------------------------------------------------------
main.o: structures.h fonctions.h
groupes.o: structures.h fonctions.h
pilotes.o: structures.h

#---------------------------------
# REGLES DE COMPILATION IMPLICITES
#---------------------------------
%.o : %.c ; $(CXX) $(CXXFLAGS) $*.c

