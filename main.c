#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/****************************************************************
 * Ce projet a pour objectif de simuler le système industriel de Sy15
 * Voir le README.md pour plus d'informations
 * *************************************************************/

// Paramètres de simulation //
/**************************************************************/
#define STOCK_MAX_PROD1 64
#define STOCK_MAX_WAREHOUSE 64
#define STOCK_MAX_CLIENT2 64
#define HORIZON 2000
#define TAILLE_ECHEANCIER 3
// En réalité on pourait utiliser 2 mais avec un
// échéancier on peut facilement ajouter des évènements
#define TEMPS_TRAITEMENT_AGV 7
// Avant de commencer une opération les agvs ont un temps de
// traitement d'environ 7 secondes à chaques fois

/* Variables globales */
/**************************************************************/
// Variables de simulations
float t; // Temps courant
float H; // horizons de simulation

// Les Loi de la simulation
float LAW[2][3][2] = {{{10, 0.1}, {10, 0.1}, {10, 0.1}}, {{10, 0.1}, {10, 0.1}, {10, 0.1}}};
// Les lois sont organisé comme suit:
// AGV1
// 		Chargement
// 			Moyenne
// 			Ecart-type
// 		Déchargement
// 			Moyenne
// 			Ecartype
// 		Déplacement
// 			Moyenne
// 			Ecartype
// 	AGV2
// 		...
//
// Par exemple pour accéder à l'écart type de l'AGV1 en déplacement
// il faut utiliser LAW[0][2][1]

// Tableau des commandes à t=0;
int TAB_COMMANDES[STOCK_MAX_PROD1];
int NBR_COMMANDES; // Nombre total de commande après génération
int NBR_PRODUITS; // Nombre total de produit après génération

int Stock[3];
// Donne le nombre de commandes dans chaques zones de stockage
// 0: Prod1, 1: Warehouse, 2: Client2

int NBR_EVENT;
float Tab[4][TAILLE_ECHEANCIER] = {0};
// L'échéancier:
// Evenement
// date
// AGV
// lieu
//
// Les évènements sont:
// 1: Fin chargement(AGV),
// 2: Fin déchargement(AGV),
// 3: Fin déplacement(AGV, lieu)

int state[2][2];
// Les états des AGVs
//		Etat Chargement
// AGV1
// AGV2
// 0: attente, 1: dechargement, 2:chargement, 3: déplacement
//
// Par exemple pour utiliser l'état actuel de AGV2 il faut utiliser
// state[1][0]

int Files_FIFO[3][STOCK_MAX_PROD1];
// Les commandes des files

/* Fonctions de probabilités */
/**************************************************************/
int X0 = 1; // Pour U(A,B)
float U(float A, float B)
{
	// retourne une valeur aléatoire uniformément
	// répartie entre A et B, des réels
	int alpha = 15;
    int beta = 7;
    int gamma = 15654;

    X0 = (alpha*X0+beta)%(gamma);
    float val = (float) X0/gamma;
    return (float) val*(B-A)+A;
}

float Exp(float lambda)
{
	// Retourne une valeur de loi expo pour un paramètre
	// Lambda, donc de moyenne 1/lambda
	float R = U(0, 1);
	return (float)-1 * logf(R) / lambda;
}

float N(float M, float O)
{
	float sum = 0;
	int n = 10;
	for (int i = 1; i < n; i++)
	{
		sum = sum + U(0, 1);
	}
	float R = (float)(sqrtf(12) * (sum - (n / 2))) / sqrtf(n);
	return (float)R * O + M;
}

// Fonctions Utilitaires //
/**************************************************************/
void ajouter(int event, float date, int agv, int lieu)
{
	// Ajoute l'évènement dans l'échéancier
	if (NBR_EVENT == 0)
	{
		// L'évènement 0 n'existe que lorsque l'échéancier est
		// initialisé, donc c'est une vérification pour les
		// premiers ajouts
		Tab[0][0] = event;
		Tab[1][0] = date;
		Tab[2][0] = agv;
		Tab[3][0] = lieu;
	}
	else
	{
		// Recherche de position
		int pos = 0;
		while (Tab[1][pos] < date && Tab[0][pos] != 0)
		{
			pos++;
		}

		// décalage
		for (int i = 0; i < 4; i++)
		{
			for (int j = TAILLE_ECHEANCIER - 1; j > pos; j--)
			{
				Tab[i][j] = Tab[i][j - 1];
			}
		}

		// Insertion
		Tab[0][pos] = event;
		Tab[1][pos] = date;
		Tab[2][pos] = agv;
		Tab[3][pos] = lieu;
	}
	NBR_EVENT++;
}

void deletion()
{
	// Supprime le premier évènement de l'échéancier
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < TAILLE_ECHEANCIER - 1; j++)
		{
			Tab[i][j] = Tab[i][j + 1];
		}
	}
	Tab[0][TAILLE_ECHEANCIER - 1] = 0;
	Tab[1][TAILLE_ECHEANCIER - 1] = 0;
	Tab[2][TAILLE_ECHEANCIER - 1] = 0;
	Tab[3][TAILLE_ECHEANCIER - 1] = 0;
	NBR_EVENT--;
}

void init_commandes()
{
	// L'objectif de cette fonction est de générer le tableau
	// de commandes TAB_COMMANDES.
	// Ce tableau est un vecteur remplit avec le nombre de
	// commandes et le nombre de produit en FIFO
	// Le nombre max de produits et 64.

	// Par expérience, les 3-4 premières itérations de U(A,B)
	// donne 1 produit donc on fait tourner l'algorithme pour
	// avancer dans les générations
	for (int i = 0; i < 4; i++)
	{
		U(0, 1);
	}

	int compt = 0;
	int prod = (int) U(1, 7);
	NBR_COMMANDES = 0;
	NBR_PRODUITS = 0;
	while (NBR_PRODUITS + prod < STOCK_MAX_PROD1)
	{
		TAB_COMMANDES[NBR_COMMANDES] = prod;
		Files_FIFO[0][NBR_COMMANDES] = prod;
		NBR_COMMANDES++;
		NBR_PRODUITS = NBR_PRODUITS + prod;
		prod = (int) U(1, 7);
	}
}

void initialisation()
{
	// Initialise les variables de la simulation
	// A n'utiliser qu'une fois

	// Variables de simulation
	t = 0;
	H = HORIZON;
	NBR_EVENT = 0;

	// Tableau des commandes
	init_commandes();

	// Réinitialise l'écheancier
	for (int i=0; i<4; i++) {
		for (int j=0; j<TAILLE_ECHEANCIER; j++) {
			Tab[i][j] = 0;
		}
	}

	// Ajoute les initialisateurs
	ajouter(3, t + N(LAW[0][2][0], LAW[0][2][1]), 1, 1);

	state[0][0] = 0;
	state[0][1] = 0;
	state[1][0] = 0;
	state[1][1] = 0;

	Stock[0] = NBR_COMMANDES;
	Stock[1] = 0;
	Stock[2] = 0;
}

// Évènements //
/**************************************************************/
void fin_chargement(int agv)
{
	int i;
	if (agv == 1)
	{
		state[0][1] = Files_FIFO[0][0];
		for (i = 0; i <= Stock[0] - 2; i++)
		{
			Files_FIFO[0][i] = Files_FIFO[0][i + 1];
		}
		Stock[0]--;
		state[0][0] = 3;
		ajouter(3, t + N(LAW[0][2][0], LAW[0][2][2]), 1, 2);
	}
	else
	{
		state[1][1] = Files_FIFO[1][0];
		for (i = 0; i <= Stock[1] - 2; i++)
		{
			Files_FIFO[1][i] = Files_FIFO[1][i + 1];
		}
		Stock[1]--;
		state[1][0] = 3;
		ajouter(3, t + N(LAW[1][2][0], LAW[1][2][1]), 2, 3);
		if (state[0][0] == 0 && state[0][1] != 0)
		{
			state[0][0] = 1;
			ajouter(2, t + N(LAW[0][1][0], LAW[0][1][1]), 1, 0);
		}
	}
}

void fin_dechargement(int agv)
{
	if (agv == 1)
	{
		Files_FIFO[1][Stock[1]] = state[0][1];
		Stock[1]++;
		state[0][1] = 0;
		state[0][0] = 3;
		ajouter(3, t + N(LAW[0][2][0], LAW[0][2][1]), 1, 1);
		if (state[1][0] == 0)
		{
			state[1][0] = 2;
			ajouter(1, t + N(LAW[1][0][0], LAW[1][0][1]), 2, 0);
		}
	}
	else
	{
		Files_FIFO[2][Stock[2]] = state[1][1];
		Stock[2]++;
		state[1][1] = 0;
		state[1][0] = 3;
		ajouter(3, t + N(LAW[1][2][0], LAW[1][2][1]), 2, 2);
	}
}

void fin_deplacement(int agv, int lieu)
{
	if (agv == 1)
	{
		if (lieu == 1)
		{
			if (Stock[0] >= 1)
			{
				state[0][0] = 2;
				ajouter(1, t + N(LAW[0][1][0], LAW[0][1][1]), 1, 0);
			}
			else
			{
				state[0][0] = 0;
				ajouter(1, 2 * H, 1, 1);
			}
		}
		else
		{
			if (state[1][0] == 2)
			{
				state[0][0] = 0;
			}
			else
			{
				state[0][0] = 1;
				ajouter(2, t + N(LAW[0][0][0], LAW[0][0][1]), 1, 0);
			}
		}
	}
	else
	{
		if (lieu == 2)
		{
			if (state[0][0] == 2)
			{
				state[1][0] = 0;
			}
			else
			{
				if (Stock[1] >= 1)
				{
					state[1][0] = 2;
					ajouter(1, t + N(LAW[1][0][0], LAW[1][0][1]), 2, 0);
				}
				else
				{
					state[1][0] = 0;
				}
			}
		}
		else
		{
			state[1][0] = 1;
			ajouter(2, t + N(LAW[1][1][0], LAW[1][1][1]), 2, 0);
		}
	}
}

/* Fonctions d'affichages */
/**************************************************************/
void show_commandes()
{
	for (int i = 0; i < NBR_COMMANDES; i++)
	{
		printf("Commande %d: %d produits\n", i + 1, TAB_COMMANDES[i]);
	}
}

void show_echeancier()
{
	int i, j;
	printf("\n");
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < TAILLE_ECHEANCIER; j++)
		{
			printf("%f ", Tab[i][j]);
		}
		printf("\n");
	}
}

void show_state()
{
	// printf("Temps courant: %f\n", t);
	printf("Prochain evenement:\n");
	if (Tab[0][0] == 3.0)
	{
		printf("Fin de deplacement");
	}
	else if (Tab[0][0] == 2.0)
	{
		printf("Fin de dechargement");
	}
	else
	{
		printf("Fin chargement");
	}
	printf("\n");
	printf("A la date: %f", Tab[1][0]);
	printf("\n");
	printf("Avec AGV: %f", Tab[2][0]);
	printf("\n");
	printf("En: \n");
	if (Tab[3][0] == 1.0)
	{
		printf("Prod1");
	}
	else if (Tab[3][0] == 2.0)
	{
		printf("Warehouse");
	}
	else if (Tab[3][0] == 3.0)
	{
		printf("Client 2");
	}
	else
	{
		printf("---\n");
	}
	printf("\n");

	printf("Etat de AGV1: %d, %d\n", state[0][0], state[0][1]);
	printf("Etat de AGV2: %d, %d\n", state[1][0], state[1][1]);

	printf("Nbr commandes Prod1: %d\n", Stock[0]);
	printf("Nbr commandes Warehouse: %d\n", Stock[1]);
	printf("Nbr commandes Client2: %d\n", Stock[2]);

	printf("Etat de l'echeancier: \n");
	show_echeancier();
}

void start_display()
{
	printf(
		"##################################" //34 #
		"\n#                                #"
		"\n#   SY15: Projet de simulation   #"
		"\n#                                #"
		"\n# 1. Lancer la simulation        #"
		"\n# 2. Lancer simulation (detail)  #"
		"\n# 3. Initialiser la simulation   #"
		"\n# 4. Modifier parametres         #"
		"\n# 5. Voir parametres simulation  #"
	 	"\n# 6. Voir les commandes          #"
	 	"\n# 7. Quitter                     #"
		"\n#                                #"
		"\n##################################"

	);
	printf("\n");
}

void afficher_commandes()
{
	printf("\n##################################"); //34 #
	printf("\n#                                #");
	printf("\n#       Liste des commandes      #");
	printf("\n#                                #");
	printf("\n##################################"); //34 #
	for (int i=0; i<NBR_COMMANDES; i++)
	{
		printf(
			"\nCommande %d, produits: %d",
			i + 1, TAB_COMMANDES[i]
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
	printf("\nStock max Warehouse: %d\n", STOCK_MAX_WAREHOUSE);

	printf(
		"\nParametres des AGV:\n"
		"AGV1:\n"
		"\tmoyenne de chargement: %.2f\n"
		"\tecart-type de chargement: %.2f\n"
		"\tmoyenne de dechargement: %.2f\n"
		"\tecart-type de dechargement: %.2f\n"
		"\tmoyenne de deplacement: %.2f\n"
		"\tecart-type de deplacement: %.2f\n"
		"AGV2:\n"
		"\tmoyenne de chargement: %.2f\n"
		"\tecart-type de chargement: %.2f\n"
		"\tmoyenne de dechargement: %.2f\n"
		"\tecart-type de dechargement: %.2f\n"
		"\tmoyenne de deplacement: %.2f\n"
		"\tecart-type de deplacement: %.2f\n",
		LAW[0][0][0], LAW[0][0][1], LAW[0][1][0], LAW[0][1][1],
		LAW[0][2][0], LAW[0][2][1],
		LAW[1][0][0], LAW[1][0][1], LAW[1][1][0], LAW[1][1][1],
		LAW[1][2][0], LAW[1][2][1]
	);
}

// Algorithme principale //
/**************************************************************/
int algo_principal(int verbose)
{
	t = 0; // reinitialise le temps pour plusieurs simulations
	int ev, agv, lieu, i, j;
	// initialisation();
	system("clear");
	printf("\n--- Debut de simulation ---\n\n");
	while (t < H && Stock[2] < NBR_COMMANDES)
	{
		ev = Tab[0][0];
		t = Tab[1][0];
		agv = Tab[2][0];
		lieu = Tab[3][0];
		if (verbose == 1)
		{ // Affiches l'état du système
			printf("\n--- Nouveau cycle --- \n");
			show_state();
		}
		deletion();
		if (ev == 1) // Sélectionne l'évènement
		{
			fin_chargement(agv);
		}
		else if (ev == 2)
		{
			fin_dechargement(agv);
		}
		else
		{
			fin_deplacement(agv, lieu);
		}
	}

	printf("\n--- Fin de simulation ---\n\n");
	printf("Temps de simulation: %f\n", t);

	return 0;
}

void menu()
{
	/*
	 * Quatres choix de simulations
	 * 1. Lancer la simulation
	 * 2. Lancer la simulation avec détails
	 * 3. Initialiser la simulation
	 * 4. Changer parametres de simulation
	 * 5. Voir les paramètres de simulation
	 * 6. Voir les commandes à t=0;
	 * 7. Quitter
	 */
	int choix = -1; // Choix du user
	int wait; // Pour attendre la frappe
	int init=0; // A t=0 la simu n'est pas initialise
	system("clear"); //allow to clear the cmd
	start_display(); // Affiche l'écran d'acceuil
	while (choix != 7)
	{
		scanf("%d", &choix);
		if (choix == 1)
		{
			// 1. Lancer la simulation
			if (init==0)
			{
				// Simulation non initialise
				printf("\n/!\\ Simulation non initialise /!\\\n");
				fflush(stdin);
				scanf(">>%d", &wait);
			} else {
				// Simulation initialise
				fflush(stdin);
				choix = algo_principal(0);
				// L'argument 0 n'affiche pas la simulation en temps
				// reel
				fflush(stdin);
				scanf(">>%d", &wait);
			}
		} else if (choix == 2) {
			// 1. Lancer la simulation
			if (init==0)
			{
				// Simulation non initialise
				printf("\n/!\\ Simulation non initialise /!\\\n");
				fflush(stdin);
				scanf(">>%d", &wait);
			} else {
				// Simulation initialise
				fflush(stdin);
				choix = algo_principal(1);
				// L'argument 0 n'affiche pas la simulation en temps
				// reel
				fflush(stdin);
				scanf(">>%d", &wait);
			}
		} else if (choix == 3) {
			printf("\nSimulation initialise !\n");
			initialisation();
			init = 1;
			fflush(stdin);
			scanf(">>%d", &wait);
		} else if (choix == 4) {
			// R
			fflush(stdin);
			scanf("%d", &wait);
		} else if (choix == 5) {
			afficher_parametres_simu();
			fflush(stdin);
			scanf(">>%d", &wait);
		} else if (choix == 6) {
			afficher_commandes();	
			fflush(stdin);
			scanf(">>%d", &wait);
		}
		system("clear");
		start_display();
	}
	system("clear");
}

int main(int argc, char *argv[])
{
	// Sans argument, lance la simulation classique
	// avec '-t', lance en mode test
	// Avec '-d' lance en mode dev
	//
	// i, j et k sont des compteurs de test

	system("clear"); // nettoie la console

	if (argc == 1)
	{
		menu(); // Lance la simulation
		return 0;
	}
	else if (strcmp(argv[1], "-d") == 0)
	{
		// Lance en mode dev pour avoir
		// toutes les infos de simulation
		printf("\n --- Dev ---\n\n");
		initialisation();
		algo_principal(1);
		return 0;
	}
	else
	{
		// Si on transmet un argument qui n'existe pas
		printf("Error: No matching argument found\n");
		return -1;
	}
}
