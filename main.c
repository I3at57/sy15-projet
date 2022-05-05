#include <stdio.h>
#include <stdlib.h>

/****************************************************************
 * Ce projet a pour objectif de simuler le système industriel de Sy15
 * Voir le README.md pour plus d'informations
 * *************************************************************/

/* Le paramètres constants de la simulation */
#define STOCK_MAX_PROD1 64
#define STOCK_MAX_WAREHOUSE 64
#define STOCK_MAX_CLIENT2 64

/* Variables globales */
int X0=1; // Pour U(A,B)

// Le stock maximal de Prod 1 est de 64
// Donc il peut au maximum y avoir 64 commandes de 1 produit
int ARRAY_COMMANDES[STOCK_MAX_PROD1];
int NBR_COMMANDES;
int NBR_PRODUITS;


/* Fonctions de probabilités */
float U(float A, float B)
{
	// retourne une valeur aléatoire uniformément
	// répartie entre A et B, des réels
	int alpha = 7;
    int beta = 98;
    int gamma = 1331;

    X0 = (alpha*X0+beta)%(gamma);
    float val = (float) X0/gamma;
    return (float) val*(B-A)+A;
}

/* Fonctions d'affichages */
void start_display()
{
	system("clear"); //allow to clear the cmd
	printf(
		"##################################" //34 #
		"\n#                                #"
		"\n#   SY15: Projet de simulation   #"
		"\n#                                #"
		"\n# 1. Lancer la simulation        #"
		"\n# 2. Voir parametres simulation  #"
	 	"\n# 3. Voir les commandes          #"
	 	"\n# 4. Quitter                     #"
		"\n#                                #"
		"\n##################################"

	);
	printf("\n");
}

void afficher_commandes()
{
	printf("\n##################################"); //34 #
	printf("\n#                                #");
	printf("\n#    Generation des commandes    #");
	printf("\n#                                #");
	printf("\n##################################"); //34 #
	for (int i=0; i<NBR_COMMANDES; i++)
	{
		printf(
			"\nCommande %d, produits: %d",
			i+1, ARRAY_COMMANDES[i]
		);
	}
	printf(
		"\nTotal commandes: %d\nTotal produits: %d\n",
		NBR_COMMANDES, NBR_PRODUITS
	);
}

void afficher_parametres_simu()
{
	printf("\n##################################"); //34 #
	printf("\n#                                #");
	printf("\n#    Parametres de simulation    #");
	printf("\n#                                #");
	printf("\n##################################"); //34 #

	printf("\nStock max Prod1: %d", STOCK_MAX_PROD1);
	printf("\nStock max Client2: %d", STOCK_MAX_CLIENT2);
	printf("\nStock max Warehouse: %d", STOCK_MAX_WAREHOUSE);
	printf("\n");
}
/* Fonctions intermédiaires */
void initialiser_commandes()
{
	int total = 0;
	int nbr_commandes = 0;
	int nbr_prod;
	while (total <= 58)
	{
		nbr_prod = (int) U(1, 6);
		ARRAY_COMMANDES[nbr_commandes] = nbr_prod;
		nbr_commandes++;
		total = total + nbr_prod;
	}
	NBR_COMMANDES = nbr_commandes;	
	NBR_PRODUITS = total;
}

int main()
{
	/*
	 * Quatres choix de simulations
	 * 1. Lancer la simulation
	 * 2. Voir les paramètres de simulation
	 * 3. Voir les commandes à t=0;
	 * 4. Quitter
	 */
	initialiser_commandes();
	int choix = 5;
	int wait;
	start_display();
	while (choix != 4)
	{
		scanf("%d", &choix);
		fflush(stdin);
		start_display();
		if (choix == 1)
		{
			// R
		} else if (choix == 2) {
			afficher_parametres_simu();
			fflush(stdin);
			scanf(">>%d", &wait);
		} else if (choix == 3) {
			afficher_commandes();	
			fflush(stdin);
			scanf(">>%d", &wait);
		}
	}
	system("clear");
	return 0;
}
