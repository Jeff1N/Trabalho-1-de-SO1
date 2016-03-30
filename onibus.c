//Jefferson Boldrin Cardozo - 7591671

#include <pthread.h>	//Para pthreads. Compilar com "gcc -pthreads -o 'output' 'arquivo.c'"
#include <unistd.h>		//Para sleep()
#include <sys/queue.h>	//Para tad fila

#include <stdio.h>
#include <stdlib.h>

//Structs e Typedefs
	//TAD Queue
	struct Node {
		int item;
		struct Node* next;
	};
	typedef struct Node Node;	

	struct Queue {
		Node* head;
		Node* tail;
		int size;	
	};
	typedef struct Queue Queue;

	//Structs de Parada, Ônibus e Passageiro
	struct Stop {
		pthread_t thread;		//A thread processando a parada atual
		
		int carro;				//Valor negativo indica que não há nenhum carro atualmente nesta parada
		Queue passEsperando;	//Fila de passageiros esperando no ponto de ônibus
	};
	typedef struct Stop Stop;

	struct Carro {
		pthread_t thread;

		int estado;			//0 = parado, passageiros descendo; 1 = parado, passageiros subindo; 2 = na estrada
		int stop;			//Se parado, mostra parada atual. Se em movimento, mostra próxima parada
		int nCarro;			//Numero do carro atual

		Queue passBordo;	//Fila com passageiros a bordo deste carro
		int passNDesc;		//Passageiros que informaram que não pretendem descer do ônibus

		int tempoProxStop;	//Tempo faltando para a próxima parada
	};
	typedef struct Carro Carro;

	struct Passageiro {
		pthread_t thread;
		
		int viajando;			//1 = passageiro ainda não chegou no destino; 0 = já chegou
		int onibus;				//-1 = não está num onibus; n = no do onibus atual
		int ptoPartida;		
		int ptoChegada;
	};	
	typedef struct Passageiro Passageiro;

//Variáveis globais
	int terminou = 0;
	int passAindaViajando;

	Stop 		*stops;
	Carro 		*carros;
	Passageiro 	*passageiros;

	pthread_t renderer; 	//Thread que cuida da animação

	int S, C, P, A; //S = bus stops, C = carros, P = passageiros, A = assentos

//Declaração de funções
	void Entrada();			//Recebe e trata a entrada das variáveis S, C, P e A
	void Inicializacao();	//Inicializa as structs e a thread de cada membro
	
	void *Animacao(void *argAnimacao);			//Função da thread de animação
	void *funcStop(void *argStop);				//Função das threads de Paradas
	void *funcCarro(void *argCarro);			//Função das threads de Carros
	void *funcPassageiro(void *argPassageiro);	//Função das threads de Passageiros

	//TAD Queue
	Queue createQueue ();
	void push(Queue* queue, int item);
	int pop(Queue* queue);
	int peek(Queue* queue);
	void display(Queue* queue);

//Main
	int main (){
		
		Entrada();
		Inicializacao();

		while (terminou != 1)
			terminou *= -1;

		return 0;
	}

//Funções Sequenciais
	void Entrada(){
		printf("Numero de paradas (S): ");
		scanf("%d", &S);
		while (S <= 0){
			printf("S deve ser um numero positivo e maior que zero, digite novamente: ");
			scanf("%d", &S);
		}

		printf("Numero de carros (C): ");
		scanf("%d", &C);
			while (C <= 0 || C > S){
			printf("C deve ser um numero positivo, maior que zero e menor ou igual a S, digite novamente: ");
			scanf("%d", &C);
		}

		printf("Numero de passageiros (P): ");
		scanf("%d", &P);
		while (P <= 0 || P <= C){
			printf("P deve ser um numero positivo, maior que zero e maior ou igual C, digite novamente: ");
			scanf("%d", &P);
		}

		printf("Numero de assentos (A): ");
		scanf("%d", &A);
			while (A <= 0 || A <= C || A >= P){
			printf("A deve ser um numero positivo, maior que zero, maior ou igual C e menor ou igual a P, digite novamente: ");
			scanf("%d", &A);
		}
		
		passAindaViajando = P;
	}

	void Inicializacao(){
	//Inicializa arrays das structs Stop, Carro, Passageiro
		stops 		= malloc(sizeof(Stop) 		* S);
		carros 		= malloc(sizeof(Carro) 		* C);
		passageiros = malloc(sizeof(Passageiro) * P);

		int i;
		//Inicializa Stops
		for (i = 0; i < S; ++i){
			stops[i].carro = -1; 			//Valor neg indica nenhum carro
			stops[i].passEsperando = createQueue();
		}
		//Inicialize Passageiros
		srand(time(NULL));
		for (i = 0; i < P; ++i){
			passageiros[i].viajando = 1;

			passageiros[i].ptoPartida = rand() % (S-1);
			passageiros[i].ptoChegada = rand() % (S-1);

			if (passageiros[i].ptoPartida == passageiros[i].ptoChegada){
				if (passageiros[i].ptoPartida == S-1) passageiros[i].ptoChegada = 0;
				else passageiros[i].ptoPartida++;
			}

			push(&stops[passageiros[i].ptoPartida].passEsperando, i);
		}
		//Inicializa Carros
		for (i = 0; i < C; ++i){
			carros[i].estado = 0;
			carros[i].nCarro = i;
			carros[i].passBordo = createQueue();

			carros[i].stop = rand() % (S-1);

			while (stops[carros[i].stop].carro != -1) {
				carros[i].stop++;
				if (carros[i].stop >= S) carros[i].stop = 0;
			}
			stops[carros[i].stop].carro = i;
		}

	//Inicializa threads
		//Thread de Animação
		if(pthread_create(&renderer, NULL, Animacao, NULL)) {
			fprintf(stderr, "Erro ao criar a thread de animacao, interrompendo programa.\n");
			terminou = 1;
			return;
		}
		//Threads de Paradas
		for (i = 0; i < S; ++i){
			if(pthread_create(&stops[i].thread, NULL, funcStop, (void *) &stops[i] )) {
				fprintf(stderr, "Erro ao criar uma thread de parada de onibus, interrompendo programa.\n");
				terminou = 1;
				return;
			}
		}
		//Threads de Passageiros
		for (i = 0; i < P; ++i){
			if(pthread_create(&passageiros[i].thread, NULL, funcPassageiro, (void *) &passageiros[i] )) {
				fprintf(stderr, "Erro ao criar uma thread de passageiro, interrompendo programa.\n");
				terminou = 1;
				return;
			}
		}		
		//Threads de Carros
		for (i = 0; i < C; ++i){
			if(pthread_create(&carros[i].thread, NULL, funcCarro, (void *) &carros[i] )) {
				fprintf(stderr, "Erro ao criar uma thread de parada de carro, interrompendo programa.\n");
				terminou = 1;
				return;
			}
		}
	}

//Funções paralelas
	void *Animacao(void *argAnimacao){
		while (terminou != 1){
			printf("\033[H\033[J");

			printf("Paradas: \n\n");
			int i;
			for (i = 0; i < S; i++){
				printf("(Parada %d - Passageiros em espera: %d)\t", i, stops[i].passEsperando.size);
				if (stops[i].carro >= 0) printf("(Carro %d - Passageiros a bordo: %d)\n", stops[i].carro, carros[stops[i].carro].passBordo.size);
				else printf("(S/ Carro)\n");
			}
			
			printf("\nOnibus na estrada: \n\n");
			for (i = 0; i < C; i++){
				if (carros[i].estado == 2) 
				printf("Carro %d (prox S: %d) (Ps a bordo: %d)\n", i, carros[i].stop, carros[i].passBordo.size);
			}
			
			/*printf("\nPassageiros:\n\n");
			for (i = 0; i < P; i++){
				printf("Passageiro %d: %d -> %d\n", i, passageiros[i].ptoPartida, passageiros[i].ptoChegada);
			}*/
			
			sleep(2);
		}
	}

	void *funcStop(void *argStop){
		
	}

	void *funcCarro(void *objCarro){
		Carro *tCarro = (Carro *) objCarro;

		while (!terminou){
			//Se o carro está parado, espera passageiros (des)embarcarem e então parte para a próxima parada.
			if (tCarro->estado == 0){
				//if (tCarro->passNDesc == )
			}

			else if (tCarro->estado == 1){
				if (stops[tCarro->stop].passEsperando.size > 0){
					push(&(tCarro->passBordo), pop(&stops[tCarro->stop].passEsperando) );
				}
				else{
					tCarro->estado = 1;
					stops[tCarro->stop].carro = -1;

					
					tCarro->stop++;
					if(tCarro->stop >= S) tCarro->stop = 0;
					
					tCarro->tempoProxStop = (rand() % 1000)+50;
				}
			}

			//Se o carro está em movimento, diminui gradualmente o tempo para a próxima parada.
			//Ao chegar lá, se a parada estiver vaga, estaciona.
			else if ((tCarro->estado == 2)){
				if (tCarro->tempoProxStop > 0){
					tCarro->tempoProxStop--;
				}
				else{
					tCarro->estado = 0;
					//stops[tCarro->stop].carro
				}
			}
		} 
	}

	void *funcPassageiro(void *argPassageiro){
		
	}

//Funções do TAD Queue
	void push (Queue* queue, int item) {
		Node* n = (Node*) malloc (sizeof(Node));
		n->item = item;
		n->next = NULL;

		if (queue->head == NULL) {
			queue->head = n;
		}
		else{
			queue->tail->next = n;
		}
		queue->tail = n;
		queue->size++;
	}

	int pop (Queue* queue) {
		Node* head = queue->head;
		int item = head->item;
		queue->head = head->next;
		queue->size--;
		free(head);
		return item;
	}

	int peek (Queue* queue) {
		Node* head = queue->head;
		return head->item;
	}

	void display (Queue* queue) {
		printf("\nDisplay: ");
		if (queue->size == 0) printf("No item in queue.\n");
		else {
			Node* head = queue->head;
			int i, size = queue->size;
			printf("%d item(s):\n", queue->size);
			for (i = 0; i < size; i++) {
				if (i > 0) printf(", ");
				printf("%d", head->item);
				head = head->next;
			}
		}
		printf("\n\n");
	}

	Queue createQueue () {
		Queue queue;
		queue.size = 0;
		queue.head = NULL;
		queue.tail = NULL;
		return queue;
}

