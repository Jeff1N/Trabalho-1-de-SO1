//Jefferson Boldrin Cardozo - 7591671

#include <pthread.h>	//Para pthreads. Compilar com "gcc -pthreads -o 'output' 'arquivo.c'"
#include <unistd.h>		//Para sleep() da animação
#include <semaphore.h>  //Para o semáforo
#include <time.h>       //Para contagem de tempo

#include <stdio.h>
#include <stdlib.h>

//Structs e Typedefs
	//TAD Queue
	typedef struct Node {
		int item;
		struct Node* next;
	} Node;

	typedef struct Queue {
		Node* head;
		Node* tail;
		int size;
	} Queue;

	//Structs de Parada, Ônibus e Passageiro
	typedef struct Stop {
		pthread_t thread;		//Thread que controla esta saída

        Queue passEsperando;	//Fila de passageiros esperando no ponto de ônibus

        int nStop;              //Número do ponto atual
		int carro;				//-1 = nenhum carro atualmente nesta parada; n = no do onibus atualmente neste ponto
	} Stop;

	typedef struct Carro {
		pthread_t thread;   //Thread que controla este carro

		int estado;			//0 = parado, passageiros descendo; 1 = parado, passageiros subindo; 2 = na estrada

		int nCarro;			//Numero do carro atual
        int stop;			//Se parado, mostra parada atual. Se em movimento, mostra próxima parada

		int contPass;		//Contador de passageiros no ônibus
        int contPassNDesc;  //Contador de passageiros que confirmaram que não descem no ponto atual

        time_t horaSaida;   //Tempo que o onibus saiu do ultimo ponto
		int tempoProxStop;	//Tempo necessário para a próxima parada (em segundos)
	} Carro;

	typedef struct Passageiro {
		pthread_t thread;       //Thread que controla este passageiro
		FILE *trace;            //Arquivo de saída deste passageiro

		int viajando;			// 1 = passageiro ainda não chegou no destino; 0 = já chegou

		int nPass;              //Número do passageiro
		int onibus;				//-1 = não está num onibus; n = no do onibus atual
		int ponto;              //-1 = não está num ponto;  n = no do ponto atual

		int ptoPartida;
		int ptoChegada;

		time_t horaChegada;     //Tempo que o passageiro chegou no ponto de destino
        int tempoEspera;        //Quanto tempo o passageiro pretende passar no ponto de destino

		enum estado {ptoPart = 0, indoDest = 1, Dest = 2, voltDest = 3, fim = 4} estado;
	}Passageiro;



//Variáveis globais
	int S, C, P, A;         //S = bus stops, C = carros, P = passageiros, A = assentos

	sem_t mutex;            //Semáforo
	int terminou = 0;       //Variável que controla se a simulação já terminou ou não
	int passAindaViajando;  //Nº de passageiros ainda viajando
	time_t tempo;           //Variável utilizada com a função time() para dizer o tempo atual

    pthread_t renderer; 	//Thread que cuida da animação

    //Arrays de structs das entidades da simulação
	Stop 		*stops;
	Carro 		*carros;
	Passageiro 	*passageiros;



//Declaração de funções
	void Entrada();			//Recebe e trata a entrada das variáveis S, C, P e A
	void Execucao();        //Mantém o programa executando enquanto a simulação não terminar, e encerra as ativades quando esta terminar
	void Inicializacao();	//Inicializa as structs e threads de cada entidade
    void uAnimacao();       //Mostra apenas uma tela da animação

	void *Animacao(void *argAnimacao);			//Função da thread de animação
	void *funcStop(void *argStop);				//Função das threads de Paradas
	void *funcCarro(void *argCarro);			//Função das threads de Carros
	void *funcPassageiro(void *argPassageiro);	//Função das threads de Passageiros

	//TAD Queue
	Queue createQueue ();
	void  push(Queue* queue, int item);
	int   pop (Queue* queue);

//Main
	int main (){

		Entrada();          //Recebe e trata a entrada das variáveis S, C, P e A
		Inicializacao();    //Inicializa as structs e threads de cada entidade
        Execucao();         //Mantém o programa executando enquanto a simulação não terminar, e encerra as ativades quando esta terminar

		return 0;
	}

//Funções Sequenciais
    //Mantém o programa executando enquanto a simulação não terminar.
    //Quando ela terminar, encerra as threads, destroi o semáforo e termina o programa.
    void Execucao(){
        while (passAindaViajando > 0) { }   //Mantém o programa rodando enquanto a simulação não terminar
        terminou = 1;                       //Sinaliza as threads para terminarem sua execução

        uAnimacao();

        int i;
        for (i = 0; i < S; i++){
            if(pthread_join(stops[i].thread, NULL))             fprintf(stderr, "Erro no 'join' de thread de parada.\n");   }

        for (i = 0; i < P; i++){
                if(pthread_join(passageiros[i].thread, NULL))   fprintf(stderr, "Erro no 'join' de thread de passageiro.\n");   }

        for (i = 0; i < C; i++){
                if(pthread_join(carros[i].thread, NULL))        printf(stderr, "Erro no 'join' de thread de carros.\n");    }

        sem_destroy(&mutex);
    }

    //Recebe e trata a entrada das variáveis S, C, P e A
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

    //Inicializa as structs e threads de cada entidade
	void Inicializacao(){
	//Inicializa semáforo e arrays das structs Stop, Carro, Passageiro
		sem_init(&mutex, 0, 1);

		stops 		= malloc(sizeof(Stop) 		* S);
		carros 		= malloc(sizeof(Carro) 		* C);
		passageiros = malloc(sizeof(Passageiro) * P);

		int i;
    //Inicializa Stops
		for (i = 0; i < S; ++i){
			stops[i].carro = -1; 			//Valor neg indica nenhum carro
			stops[i].passEsperando = createQueue();
			stops[i].nStop = i;
		}
    //Inicialize Passageiros
		srand(time(NULL));
		for (i = 0; i < P; ++i){
			passageiros[i].viajando = 1;
			passageiros[i].estado   = 0;
			passageiros[i].onibus   = -1;
			passageiros[i].nPass    = i;

			passageiros[i].ptoPartida = rand() % (S-1);
			passageiros[i].ptoChegada = rand() % (S-1);

			if (passageiros[i].ptoPartida == passageiros[i].ptoChegada){
				if (passageiros[i].ptoPartida == S-1) passageiros[i].ptoChegada = 0;
				else passageiros[i].ptoChegada++;
			}
			passageiros[i].ponto = passageiros[i].ptoPartida;

			push(&stops[passageiros[i].ptoPartida].passEsperando, i);

			//***Mais que 9999 passageiros irá dar erro nos arquivos de trace
			char fileName[32] = "./passageiro";
			char fileNumber[4];

			int j, k = i;
			for (j = 0; j < 4; j++){
                fileNumber[j] = '0' + (k % 10);
                k /= 10;
			}
			for (j = 12; j < 16; j++){
                fileName[j] = fileNumber[15 - j];
			}

			fileName[16] = '.';
			fileName[17] = 't';
			fileName[18] = 'r';
			fileName[19] = 'a';
			fileName[20] = 'c';
			fileName[21] = 'e';
			fileName[22] = '\0';

			//printf("%s \n", fileName);
			passageiros[i].trace = fopen(fileName, "w");

			char buff[20];
            time_t now = time(NULL);
            strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

			fprintf(passageiros[i].trace, "Horario que chegou no ponto de origem %d: %s\n",
                    passageiros[i].ptoPartida, buff);
		}
    //Inicializa Carros
		for (i = 0; i < C; ++i){
			carros[i].estado = 1;	//Inicialmente nenhum passageiro está dentro de um ônibus, então estado inicial é de passageiros embarcando
			carros[i].nCarro = i;

			carros[i].stop = rand() % (S-1);

			while (stops[carros[i].stop].carro != -1) {
				carros[i].stop++;
				if (carros[i].stop >= S) carros[i].stop = 0;
			}
			stops[carros[i].stop].carro = i;
		}

		uAnimacao();
		sleep(1);

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

    //Renderiza um "quadro" da animação.
    void uAnimacao(){
        printf("\033[H\033[J");
            printf("Passageiros ainda em viagem: %d\n\n", passAindaViajando);
            printf("Paradas (S): \n\n");
            int i;
            for (i = 0; i < S; i++){
                printf("S%d (P em espera: %d)\t", i, stops[i].passEsperando.size);
                if (stops[i].carro >= 0) {
                        printf("(C%d, P a bordo: %d, ", stops[i].carro, carros[stops[i].carro].contPass);

                        if (carros[stops[i].carro].estado == 0)         printf("Desembarcando)\n");
                        else if (carros[stops[i].carro].estado == 1)    printf("Embarcando)\n");
                }
                else printf("(S/ Carro)\n");
            }

            printf("\nOnibus (C): \n\n");
            for (i = 0; i < C; i++){
                    printf("C%d (S: ", i);
                    if (carros[i].estado == 2) printf("=> ");
                    printf("%d, P a bordo: %d, ", carros[i].stop, carros[i].contPass);

                    if (carros[i].tempoProxStop - (time(&tempo) - carros[i].horaSaida) >= 0 && carros[i].estado == 2)
                        printf("ETA: %d, ", carros[i].tempoProxStop - (time(&tempo) - carros[i].horaSaida) );

                    if      (carros[i].estado == 0) printf("Desembacando)\n");
                    else if (carros[i].estado == 1) printf("Embarcando)\n");
                    else if (carros[i].estado == 2) printf("Na estrada)\n");
            }

            printf("\nPassageiros (P):\n\n");
            for (i = 0; i < P; i++){
                printf("P%d (%d -> %d) ",  i, passageiros[i].ptoPartida, passageiros[i].ptoChegada);
                if (passageiros[i].estado == ptoPart || passageiros[i].estado == Dest)
                    printf("S: %d, ", passageiros[i].ponto);
                else if (passageiros[i].estado == indoDest || passageiros[i].estado == voltDest)
                    printf("C: %d, ", passageiros[i].onibus);

                if (passageiros[i].estado == ptoPart)           printf("No Pto de Partida\n");
                else if (passageiros[i].estado == indoDest)     printf("Indo para o Pto de Destino\n");
                else if (passageiros[i].estado == Dest){
                    if (time(&tempo) - passageiros[i].horaChegada < passageiros[i].tempoEspera)
                        printf("Passeando no Pto de Destino\n");
                    else printf("Pronto para voltar do Pto de Destino\n");
                }
                else if (passageiros[i].estado == voltDest)     printf("Voltando para o Pto de Partida\n");
                else if (passageiros[i].estado == fim)          printf("Terminou a Viagem\n");
            }
    }


//Funções paralelas
    //Função da thread de animação
	void *Animacao(void *argAnimacao){
	    while(terminou == 0){
            sem_wait(&mutex);
                uAnimacao();
            sem_post(&mutex);

            sleep(1);
        }
	}

    //Função das threads de parada
	void *funcStop(void *objStop){
        Stop *tStop = (Stop *) objStop;

        while (!terminou){
            if (tStop->carro != -1 && carros[tStop->carro].estado == 1){
                sem_wait(&mutex);

                int tamFila = tStop->passEsperando.size;
                int i, p;

                //Se o passageiro está num estado em que ele quer subir no ônibus, faz ele subir
                //Caso contrário, coloca ele de volta na fila
                for (i = 0; i < tamFila && carros[tStop->carro].contPass < A; i++){
                    p = pop(&(tStop->passEsperando));

                    if (passageiros[p].estado == ptoPart){
                        passageiros[p].estado = indoDest;
                        passageiros[p].onibus = tStop->carro;
                        carros[tStop->carro].contPass++;

                        passageiros[p].ponto = tStop->nStop;

                        char buff[20];
                        time_t now = time(NULL);
                        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

                        fprintf(passageiros[p].trace, "Horario que entrou no ônibus %d: %s\n",
                                tStop->carro, buff);
                    }

                    else if (passageiros[p].estado == Dest && time(&tempo) - passageiros[p].horaChegada >= passageiros[p].tempoEspera){
                        passageiros[p].estado = voltDest;
                        passageiros[p].onibus = tStop->carro;
                        carros[tStop->carro].contPass++;

                        passageiros[p].ponto = tStop->nStop;

                        char buff[20];
                        time_t now = time(NULL);
                        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

                        fprintf(passageiros[p].trace, "Horario que entrou no onibus %d' para voltar: %s\n",
                                tStop->carro, buff);
                    }
                    else{
                        push(&(tStop->passEsperando), p);
                    }
                }

                sem_post(&mutex);
            }
        }

        pthread_exit(0);
	}

    //Função das threads de carro
	void *funcCarro(void *objCarro){
		Carro *tCarro = (Carro *) objCarro;

		while (!terminou){
			//Se o carro está parado, espera passageiros (des)embarcarem e então parte para a próxima parada.
			if (tCarro->estado == 0){   //Desembarque
                sem_wait(&mutex);

                if (tCarro->contPassNDesc == tCarro->contPass){
                    tCarro->estado = 1;
                }

                sem_post(&mutex);
			}

			else if (tCarro->estado == 1){  //Embarque
                tCarro->contPassNDesc = 0;
				if (stops[tCarro->stop].passEsperando.size == 0 || tCarro->contPass >= A){
					sem_wait(&mutex);

					tCarro->estado = 2;
					stops[tCarro->stop].carro = -1;

					tCarro->stop++;
					if(tCarro->stop >= S) tCarro->stop = 0;

					time(&tCarro->horaSaida);
                    tCarro->tempoProxStop = (rand()%4) + 4;

					sem_post(&mutex);
				}
			}

			//Se o carro está em movimento, diminui gradualmente o tempo para a próxima parada.
			//Ao chegar lá, se a parada estiver vaga, estaciona.
			else if ((tCarro->estado == 2)){    //Viagem
				sem_wait(&mutex);

				if ( (time(&tempo) - tCarro->horaSaida) >= tCarro->tempoProxStop){
                    if (stops[tCarro->stop].carro = -1){
                        tCarro->estado = 0;
                        stops[tCarro->stop].carro = tCarro->nCarro;
                    }
                    else {
                        tCarro-> stop++;
                        if (tCarro->stop >= S) tCarro->stop = 0;

                        time(&tCarro->horaSaida);
                        tCarro->tempoProxStop = (rand()%4) + 4;
                    }
				}

				sem_post(&mutex);
			}
		}

        pthread_exit(0);
	}

    //Função das threads de passageiro
	void *funcPassageiro(void *objPass){
		Passageiro *tPass = (Passageiro *) objPass;

        while (!terminou){
            //Se passageiro está num onibus que está num ponto esperando desembarque
            if (tPass->estado == indoDest){
                if (carros[tPass->onibus].estado == 0 && tPass->ponto != carros[tPass->onibus].stop){
                    sem_wait(&mutex);
                    //Se onibus parou no ponto de destino, desce
                    if (carros[tPass->onibus].stop == tPass->ptoChegada){
                        push(&stops[tPass->ptoChegada].passEsperando, tPass->nPass);

                        carros[tPass->onibus].contPass--;

                        tPass->onibus   = -1;
                        tPass->ponto    = tPass->ptoChegada;
                        tPass->estado   = Dest;

                        time(&(tPass->horaChegada));
                        tPass->tempoEspera = (rand()%5) + 5;

                        char buff[20];
                        time_t now = time(NULL);
                        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

                        fprintf(tPass->trace, "Horario que desceu do ônibus %d no ponto de destino %d: %s\n",
                                carros[tPass->onibus].nCarro, tPass->ptoChegada, buff);
                    }
                    //Senão, espera
                    else{
                        carros[tPass->onibus].contPassNDesc++;
                        tPass->ponto = carros[tPass->onibus].stop;
                    }
                    sem_post(&mutex);
                }
            }

            else if (tPass->estado == voltDest){
                if (carros[tPass->onibus].estado == 0 && tPass->ponto != carros[tPass->onibus].stop){
                    sem_wait(&mutex);
                    //Se onibus voltou pro ponto de partida, desce
                    if (carros[tPass->onibus].stop == tPass->ptoPartida){
                        carros[tPass->onibus].contPass--;

                        tPass->onibus   = -1;
                        tPass->ponto    = tPass->ptoPartida;
                        tPass->estado   = fim;

                        passAindaViajando--;

                        char buff[20];
                        time_t now = time(NULL);
                        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

                        fprintf(tPass->trace, "Horario que desceu do ônibus %d' de volta ao ponto %d': %s\n",
                                carros[tPass->onibus].nCarro, tPass->ptoPartida, buff);
                    }
                    //Senão, espera
                    else{
                        carros[tPass->onibus].contPassNDesc++;
                        tPass->ponto = carros[tPass->onibus].stop;
                    }
                    sem_post(&mutex);
                }
            }
        }

        fclose(tPass->trace);
        pthread_exit(0);
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

	Queue createQueue () {
		Queue queue;
		queue.size = 0;
		queue.head = NULL;
		queue.tail = NULL;
		return queue;
}
