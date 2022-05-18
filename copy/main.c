#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Version brute du code de la simulation sans interface du tout
 * seulement le lancement de la simulation.
 *
 * Dans cet algorithme je choisit d'effectuer la simulation en
 */

// Paramètres de simulation //
/**************************************************************/
#define STOCK_MAX_PROD1 64
#define STOCK_MAX_WAREHOUSE 64
#define STOCK_MAX_CLIENT2 64
#define HORIZON 1200

// Variables globales //
/**************************************************************/

// Variables pour les probabilités
int X0=1;

// Variables de simulations
float t; // Temps courant
float H; // horizons de simulation


float LAW[2][3][2]={{{10, 0.1}, {10, 0.1}, {10, 0.1}}, {{10, 0.1}, {10, 0.1}, {10, 0.1}}};
// Les paramètres des lois normales
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

// Tableau des commandes à t=0;
int TAB_COMMANDES[STOCK_MAX_PROD1];
int NBR_COMMANDES;

int Stock[3];
// Donne le nombre de commandes dans chaques zones de stockage
// 0: Prod1, 1: Warehouse, 2: Client2

int NBR_EVENT;
float Tab[4][2];
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

int State[2][2];
// Les états des AGVs
//		Etat Chargement
// AGV1
// AGV2
// 0: attente, 1: dechargement, 2:chargement, 3: déplacement

int Files_FIFO[3][STOCK_MAX_PROD1];
// Les commandes des files

// Fonctions probabilistes //
/**************************************************************/
float U(float A, float B)
{
	// Retourne une valeur aléatoire suivant la loi
	// uniforme U(A, B)
	int alpha = 7;
	int beta = 98;
	int gamma= 133212;

	X0 = (alpha*X0+beta)%(gamma);
	return (float) (X0*(B-A))/gamma+A;
}

float Exp(float lambda)
{
	// Retourne une valeur de loi expo pour un paramètre 
	// Lambda, donc de moyenne 1/lambda
	float R=U(0,1);
	return (float) -1*logf(R)/lambda;
}

float N(float M, float O)
{
	float sum = 0;
	int n=10;
	for (int i=1; i<n; i++)
	{
		sum = sum + U(0, 1);
	}
	float R = (float) (sqrtf(12)*(sum-(n/2)))/sqrtf(n);
	return (float) R*O+M;
}

// Fonctions Utilitaires //
/**************************************************************/
void init_commandes()
{
	// L'objectif de cette fonction est de générer le tableau
	// de commandes TAB_COMMANDES.
	// Ce tableau est un vecteur remplit avec le nombre de
	// commandes et le nombre de produit en FIFO
	// Le nombre max de produits et 64.
	
	// Par expérience, les 3-4 premières itérations de U(A,B)
	// donne 1
	for (int i=0; i<4; i++)
	{
		U(0,1);
	}

	int compt=0;
	int prod;
	NBR_COMMANDES = 0;
	while (compt < STOCK_MAX_PROD1)
	{
		prod = (int) U(1, 7);
		TAB_COMMANDES[NBR_COMMANDES] = prod;
		Files_FIFO[0][NBR_COMMANDES] = prod;
		NBR_COMMANDES ++;
		compt = compt + prod;
	}
}

void initialisation()
{
	// Initialise les variables de la simulation
	// A n'utiliser qu'une fois
	
	// Variables de simulation
	t = 0;
	H = HORIZON;

	// Tableau des commandes
	init_commandes();

	// Ajoute les initialisateurs
	Tab[0][0] = 3;
	Tab[1][0] = t;
	Tab[2][0] = 1;
	Tab[3][0] = 1;
	NBR_EVENT = 1;

	State[0][0] = 0;
	State[0][1] = 0;
	State[1][0] = 0;
	State[1][1] = 0;

	Stock[0] = NBR_COMMANDES;
	Stock[1] = 0;
	Stock[2] = 0;
}

void ajouter(int event, float date, int agv, int lieu)
{
	if (date > Tab[1][0]) {
		Tab[0][1] = event;
		Tab[1][1] = date;
		Tab[2][1] = agv;
		Tab[3][1] = lieu;
	} else {
		Tab[0][1] = Tab[0][0];
		Tab[1][1] = Tab[1][0];
		Tab[2][1] = Tab[2][0];
		Tab[3][1] = Tab[3][0];
		
		Tab[0][0] = event;
		Tab[1][0] = date;
		Tab[2][0] = agv;
		Tab[3][0] = lieu;
		NBR_EVENT++;
	}
}

// Évènements //
/**************************************************************/
void fin_chargement(int agv)
{
	int i;
	if (agv == 1)
	{
		State[0][1] = Files_FIFO[0][0];
		for (i=0; i<=Stock[0]-2; i++)
		{
			Files_FIFO[0][i]=Files_FIFO[0][i+1];
		}
		Stock[0]--;
		State[0][0] = 3;
		ajouter(3, t+N(LAW[0][2][0], LAW[0][2][2]), 1, 2);
	} else {
		State[1][1]=Files_FIFO[1][0];
		for (i=0; i<=Stock[1]-2; i++)
		{
			Files_FIFO[1][i] = Files_FIFO[1][i+1];
		}
		Stock[1]--;
		State[1][0]=3;
		ajouter(3, t+N(LAW[1][2][0], LAW[1][2][1]), 2, 3);
		if (State[0][0] == 0 && State[0][1] != 0) {
			State[0][0]=1;
			ajouter(2, t+N(LAW[0][1][0], LAW[0][1][1]), 1, 0);
		}
	}
}

void fin_dechargement(int agv)
{
	if (agv == 1) {
		Files_FIFO[1][Stock[1]]=State[0][1];
		Stock[1]++;
		State[0][1]=0;
		State[0][0]=3;
		ajouter(3, t+N(LAW[0][2][0], LAW[0][2][1]), 1, 1);
		if (State[1][0] == 0) {
			State[1][0] = 2;
			ajouter(1, t+N(LAW[1][0][0], LAW[1][0][1]), 2, 0);
		}
	} else {
		Files_FIFO[2][Stock[2]]=State[1][1];
		Stock[2]++;
		State[1][1]=0;
		State[1][0]=3;
		ajouter(3, t+N(LAW[1][2][0], LAW[1][2][1]), 2, 2);
	}
}

void fin_deplacement(int agv, int lieu)
{
	if (agv == 1) {
		if (lieu == 1) {
			if (Stock[0] >= 1) {
				State[0][0]=2;
				ajouter(1, t+N(LAW[0][1][0], LAW[0][1][1]), 1, 0);
			} else {
				State[0][0]=0;
				ajouter(1, 2*H, 1, 1);
			}
		} else {
			if (State[1][0]==2) {
				State[0][0]=0;
			} else {
				State[0][0]=1;
				ajouter(2, t+N(LAW[0][0][0], LAW[0][0][1]), 1, 0);
			}
		}
	} else {
		if (lieu == 2) {
			if (State[0][0]==2) {
				State[1][0]=0;
			} else {
				if (Stock[1]>=1) {
					State[1][0]=2;
					ajouter(1, t+N(LAW[1][0][0], LAW[1][0][1]), 2, 0);
				} else {
					State[1][0] =0;
				}
			}
		} else {
			State[1][0]=1;
			ajouter(2, t+N(LAW[1][1][0], LAW[1][1][1]), 2, 0);
		}
	}
}

// Fonctions affichages //
/**************************************************************/
void show_commandes()
{
	for (int i=0; i<NBR_COMMANDES; i++)
	{
		printf("Commande %d: %d produits\n", i+1, TAB_COMMANDES[i]);
	}
}

void show_echeancier()
{
	int i, j;
	printf("\n");
	for (i=0; i<4; i++)
	{
		for (j=0; j<2; j++)
		{
			printf("%f ", Tab[i][j]);
		}
		printf("\n");
	}
}

void show_state()
{
	//printf("Temps courant: %f\n", t);
	printf("Prochain evenement:\n");
	if (Tab[0][0]==3.0)
	{
		printf("Fin de deplacement");
	} else if (Tab[0][0]==2.0) {
		printf("Fin de dechargement");
	} else {
		printf("Fin chargement");
	}
	printf("\n");
	printf("A la date: %f", Tab[1][0]);
	printf("\n");
	printf("Avec AGV: %f", Tab[2][0]);
	printf("\n");
	printf("En: \n");
	if (Tab[3][0]==1.0)
	{
		printf("Prod1");
	} else if (Tab[3][0]==2.0) {
		printf("Warehouse");
	} else if (Tab[3][0]==3.0) {
		printf("Client 2");
	} else {
		printf("---\n");
	}
	printf("\n");

	printf("Etat de AGV1: %d, %d\n", State[0][0], State[0][1]);
	printf("Etat de AGV2: %d, %d\n", State[1][0], State[1][1]);

	printf("Nbr commandes Prod1: %d\n", Stock[0]);
	printf("Nbr commandes Warehouse: %d\n", Stock[1]);
	printf("Nbr commandes Client2: %d\n", Stock[2]);

	printf("Etat de l'echeancier: \n");
	show_echeancier();
}

// Algorithme principale //
/**************************************************************/
void algo_principal(int verbose)
{
	int ev, agv, lieu, i, j;
	initialisation();
	while (t < H && Stock[2]<NBR_COMMANDES)
	{
		ev = Tab[0][0];
		t = Tab[1][0];
		agv = Tab[2][0];
		lieu = Tab[3][0];
		if (verbose == 1) { // Affiches l'état du système
			printf("\n--- Nouveau cycle --- \n");
			show_state();
		}
		for (j=0; j<4; j++) // Supprime l'évenement
		{
			Tab[j][0] = Tab[j][1];
		}
		NBR_EVENT--;
		if (ev == 1) // Sélectionne l'évènement
		{
			fin_chargement(agv);
		} else if (ev == 2) {
			fin_dechargement(agv);
		} else {
			fin_deplacement(agv, lieu);
		}
	}
}

int main(int argc, char *argv[])
{
	// Sans argument, lance la simulation classique
	// avec '-t', lance en mode test
	// Avec '-d' lance en mode dev
	//
	// i, j et k sont des compteurs de test

	system("clear");
	printf("%d\n", argc);
	for (int k=0; k<argc; k++)
	{
		printf("%s\n", argv[k]);
	}

	if (argc == 1)
	{
		printf("\n --- Lancement de la simulation ---\n\n");
		algo_principal(0);
		printf("\n --- Fin de la simulation ---\n\n");
		printf("Temps de simulation: %f\n", t);
		return 0;

	} else if (strcmp(argv[1], "-d") == 0) {
		printf("\n --- Dev ---\n\n");
		algo_principal(1);
		return 0;

	} else if (strcmp(argv[1], "-t") == 0) {
		printf("\n --- Test ---\n\n");
		float rea;
		float sum=0;
		int ite=20;
		for (int i=0; i<ite; i++)
		{
			//rea = Exp(5);
			rea = N(10, 0.1);
			printf("%f\n", rea);	
			sum = sum + rea;
		}
		printf("Moyenne: %f\n", (float) sum/ite);
		return 0;

	} else {
		printf("Error: No matching argument found\n"); 
		return -1;
	}
}
