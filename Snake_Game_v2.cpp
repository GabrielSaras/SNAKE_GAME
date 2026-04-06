/* =====================================================
 *  SNAKE GAME v2 - Jogo da Cobrinha em C para Windows
 * =====================================================
 *
 *  Controles - Jogador 1 : Setas direcionais
 *  Controles - Jogador 2 : W / A / S / D
 *  Pausar               : P
 *  Sair                 : ESC
 *
 *  Compilacao Dev-C++ / MinGW:
 *    gcc snake_deluxe_v3.c -o snake_deluxe.exe  (ou F11 no Dev-C++)
 *
 * =====================================================
 *  IMPLEMENTACAO DE EMOJIS NO JOGO DA COBRINHA
 * =====================================================
 *
 * 1) DEFINICAO DAS CONSTANTES DE EMOJIS
 *    Foram criadas constantes usando #define para representar
 *    as frutas, obstaculos e elementos da cobra, permitindo
 *    facil alteracao futura.
 *    Exemplo:
 *      #define FRUIT_APPLE   "\U0001F34E"  // maca vermelha
 *      #define FRUIT_GRAPE   "\U0001F347"  // uva
 *      #define FRUIT_BANANA  "\U0001F34C"  // banana
 *      #define WALL_EMOJI    "\U00002B1B"  // quadrado preto (parede)
 *      #define THORN_EMOJI   "\U0001F332"  // pinheiro (espinho)
 *
 * ---
 * 2) ATIVACAO DO UTF-8 NO CONSOLE
 *    Para que os emojis aparecam corretamente no Windows,
 *    foi necessario ativar UTF-8 no console no inicio do main():
 *      system("chcp 65001");   // Ativa UTF-8 para emojis
 *    Tambem e usada a funcao:
 *      SetConsoleOutputCP(65001);
 *      setlocale(LC_ALL, ".UTF-8");
 *
 * ---
 * 3) SUBSTITUICAO DOS SIMBOLOS PELOS EMOJIS
 *    - Em gerarFruta(), os simbolos antigos (@, $, %, etc.)
 *      foram substituidos pelos emojis das frutas usando as
 *      constantes definidas (FRUIT_APPLE, FRUIT_GRAPE, etc.).
 *    - Em desenharBorda(), o caractere de linha foi substituido
 *      pelo emoji WALL_EMOJI para dar aparencia de paredes
 *      solidas no jogo.
 *    - Em desenharEspinho(), o caractere "^" foi substituido
 *      pelo emoji THORN_EMOJI (pinheiro/espinho).
 *
 * ---
 * 4) BENEFICIOS DESTA IMPLEMENTACAO
 *    - Facil troca de emojis no futuro apenas alterando os #define.
 *    - Melhora visual do jogo com frutas coloridas e paredes
 *      representadas por emojis.
 *    - Compativel com Windows Terminal ou console com suporte UTF-8.
 *
 * ---
 * 5) ATENCAO: LARGURA DUPLA DOS EMOJIS
 *    Emojis ocupam 2 colunas no terminal (largura dupla).
 *    Por isso a borda usa um emoji por celula com gotoxy()
 *    avancando de 2 em 2. O campo interno continua usando
 *    1 coluna por celula para manter a logica de colisao.
 *
 * ===================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <string.h>
#include <locale.h>

/* ================================================================
 *  CONSTANTES DO JOGO
 * ================================================================ */
#define WIDTH        55
#define HEIGHT       20
#define TOP_OFFSET    4
#define FRUIT_COUNT   4
#define MAX_RANKING   5
#define RANKING_FILE  "snake_ranking.dat"
#define MAX_TAIL    500

/*
 * Quantidade de espinhos por dificuldade:
 *   Facil   = 0  (sem espinhos)
 *   Normal  = 4  (poucos espinhos fixos)
 *   Dificil = 8  (mais espinhos + reposicionam ao comer fruta)
 */
#define ESPINHOS_NORMAL  4
#define ESPINHOS_DIFICIL 8
#define MAX_ESPINHOS     8   /* maximo absoluto (usado para o array) */

/* ================================================================
 *  CONSTANTES DE EMOJIS
 *
 *  Cada #define mapeia um elemento do jogo para seu emoji UTF-8.
 *  Para trocar um emoji, basta alterar o valor aqui — o resto
 *  do codigo usa automaticamente a nova constante.
 *
 *  FRUTAS (usadas no array frutaEmoji[] abaixo):
 * ================================================================ */
#define FRUIT_APPLE   "\U0001F34E"  /* maca vermelha  */
#define FRUIT_GRAPE   "\U0001F347"  /* uva             */
#define FRUIT_BANANA  "\U0001F34C"  /* banana          */
#define FRUIT_CHERRY  "\U0001F352"  /* cereja          */
#define FRUIT_STAR    "\U00002B50"  /* estrela         */
#define FRUIT_BOMB    "\U0001F4A3"  /* bomba           */
#define FRUIT_BOLT    "\U000026A1"  /* raio/power-up   */
#define FRUIT_HEART   "\U00002764"  /* coracao         */

/*
 *  ELEMENTOS DO CAMPO:
 *  WALL_EMOJI  — bloco preto que forma as paredes da borda.
 *                Ocupa 2 colunas; a funcao desenharBorda()
 *                usa gotoxy() avancando de 2 em 2 por isso.
 *  THORN_EMOJI — obstaculo (espinho) que mata ao toque.
 *                Tambem tem largura dupla; gerarEspinho()
 *                armazena a coluna real e desenha com offset.
 */
#define WALL_EMOJI    "\U00002B1B"  /* quadrado preto (parede)  */
#define THORN_EMOJI   "\U0001F332"  /* pinheiro (espinho)       */

/* ================================================================
 *  PALETA DE CORES (codigos WinAPI 0-15)
 * ================================================================ */
#define COR_RESET     7
#define COR_VERDE    10
#define COR_CIANO    11
#define COR_VERMELHO  4
#define COR_VM_BRILHO 12
#define COR_AMARELO  14
#define COR_DOURADO   6
#define COR_AZUL      9
#define COR_MAGENTA  13
#define COR_CINZA     8
#define COR_BRANCO   15

/* ================================================================
 *  ENUMERACOES
 * ================================================================ */
typedef enum { PARADO=0, CIMA, BAIXO, ESQUERDA, DIREITA } Direcao;
typedef enum { MENU, SELECAO, JOGANDO, PAUSADO, FIM }      EstadoJogo;
typedef enum { UM_JOGADOR=1, DOIS_JOGADORES=2 }            ModoJogo;
typedef enum { FACIL=180, NORMAL=120, DIFICIL=60 }         Velocidade;

/* ================================================================
 *  ESTRUTURAS DE DADOS
 * ================================================================ */

/* Um par de coordenadas — reutilizado para segmentos e espinhos */
typedef struct { int x, y; } Ponto;

/*
 * Cobra — todos os dados de um jogador.
 *   corpo[0]  = cabeca (posicao atual)
 *   corpo[length-1] = cauda
 */
typedef struct {
    Ponto  corpo[MAX_TAIL];
    int    length;
    Direcao dir;
    int    cor;
    int    score;
    int    vivo;
    char   nome[21];
} Cobra;

/* Fruta — posicao e tipo (indice nos arrays de emoji/cor) */
typedef struct { int x, y, tipo; } Fruta;

/* Entrada do ranking salva em arquivo binario */
typedef struct {
    char nome[21];
    int  score;
    int  modo;
} EntradaRanking;

/* ================================================================
 *  VARIAVEIS GLOBAIS
 * ================================================================ */
Cobra      jogador[2];
Fruta      frutas[FRUIT_COUNT];
EstadoJogo estado     = MENU;
ModoJogo   modo       = UM_JOGADOR;
Velocidade velocidade = NORMAL;
int        tipoFrutaSel = 0;
HANDLE     hConsole;

/*
 * Array de espinhos — obstaculos fixos (ou semi-fixos no modo dificil).
 * numEspinhos indica quantos estao ativos no momento.
 */
Ponto espinhos[MAX_ESPINHOS];
int   numEspinhos = 0;

/*
 * Arrays de frutas — usam as constantes de emoji definidas acima.
 * Para trocar uma fruta, altere apenas o #define correspondente
 * no bloco CONSTANTES DE EMOJIS (nao precisa mexer aqui).
 *
 * NOTA: cada emoji ocupa 2 colunas no terminal. A logica de colisao
 * funciona normalmente pois compara apenas coordenadas (x,y) internas.
 */
const char *frutaEmoji[] = {
    FRUIT_APPLE,   /* maca vermelha  */
    FRUIT_GRAPE,   /* uva            */
    FRUIT_BANANA,  /* banana         */
    FRUIT_CHERRY,  /* cereja         */
    FRUIT_STAR,    /* estrela        */
    FRUIT_BOMB,    /* bomba          */
    FRUIT_BOLT,    /* raio/power-up  */
    FRUIT_HEART    /* coracao        */
};
const char *frutaNome[]  = {
    "Maca    ", "Uva     ", "Banana  ", "Cereja  ",
    "Estrela ", "Bomba   ", "Raio    ", "Coracao "
};
const int frutaCor[] = {
    COR_VM_BRILHO, COR_CIANO, COR_VERDE, COR_MAGENTA,
    COR_AMARELO,   COR_CINZA, COR_AZUL,  COR_BRANCO
};

/* ================================================================
 *  UTILITARIOS DE CONSOLE
 * ================================================================ */

void setColor(int c)       { SetConsoleTextAttribute(hConsole, c); }
void sleep_ms(int ms)      { Sleep(ms); }

void gotoxy(int x, int y) {
    COORD c = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(hConsole, c);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(hConsole, &ci);
}

/*
 * centralizarTexto()
 * Imprime 'texto' centralizado horizontalmente na linha y,
 * dentro de uma area de 'larguraTela' colunas.
 */
void centralizarTexto(int y, int larguraTela, const char *texto, int cor) {
    int x = (larguraTela - (int)strlen(texto)) / 2;
    if (x < 0) x = 0;
    setColor(cor);
    gotoxy(x, y);
    printf("%s", texto);
}

/* ================================================================
 *  RANKING (ARQUIVO BINARIO)
 * ================================================================ */

int carregarRanking(EntradaRanking r[]) {
    FILE *f = fopen(RANKING_FILE, "rb");
    if (!f) return 0;
    int n = (int)fread(r, sizeof(EntradaRanking), MAX_RANKING, f);
    fclose(f);
    return n;
}

void salvarRanking(EntradaRanking r[], int n) {
    FILE *f = fopen(RANKING_FILE, "wb");
    if (!f) return;
    fwrite(r, sizeof(EntradaRanking), n, f);
    fclose(f);
}

void inserirNoRanking(const char *nome, int score, int modoJogo) {
    EntradaRanking r[MAX_RANKING];
    int n = carregarRanking(r);

    EntradaRanking nova;
    strncpy(nova.nome, nome, 20); nova.nome[20] = '\0';
    nova.score = score;
    nova.modo  = modoJogo;

    if (n < MAX_RANKING) n++;
    r[n - 1] = nova;

    /* Bubble sort simples — array de no maximo 5 elementos */
    for (int i = n-1; i > 0 && r[i].score > r[i-1].score; i--) {
        EntradaRanking tmp = r[i]; r[i] = r[i-1]; r[i-1] = tmp;
    }
    salvarRanking(r, n);
}

/* ================================================================
 *  VERIFICACOES DE POSICAO
 *  Funcoes auxiliares que testam se uma celula (x,y) esta ocupada.
 *  Usadas tanto para gerar frutas quanto para gerar espinhos.
 * ================================================================ */

/* Retorna 1 se algum segmento de qualquer cobra ativa ocupa (x,y) */
int cobraOcupaPosicao(int x, int y) {
    for (int p = 0; p < 2; p++) {
        if (!jogador[p].vivo) continue;
        for (int i = 0; i < jogador[p].length; i++)
            if (jogador[p].corpo[i].x == x && jogador[p].corpo[i].y == y)
                return 1;
    }
    return 0;
}

/* Retorna 1 se alguma fruta ativa ocupa (x,y) */
int frutaOcupaPosicao(int x, int y) {
    for (int i = 0; i < FRUIT_COUNT; i++)
        if (frutas[i].x == x && frutas[i].y == y) return 1;
    return 0;
}

/*
 * espinhoOcupaPosicao()
 * Retorna 1 se existe um espinho na celula (x,y).
 * Usado na deteccao de colisao fatal e na geracao de novas posicoes.
 */
int espinhoOcupaPosicao(int x, int y) {
    for (int i = 0; i < numEspinhos; i++)
        if (espinhos[i].x == x && espinhos[i].y == y) return 1;
    return 0;
}

/* ================================================================
 *  ESPINHOS — GERACAO E DESENHO
 * ================================================================ */

/*
 * desenharEspinho()
 * Desenha o obstaculo usando o emoji THORN_EMOJI (pinheiro).
 *
 * IMPLEMENTACAO DE EMOJI NO ESPINHO:
 *   O emoji THORN_EMOJI tambem tem largura dupla (2 colunas).
 *   A colisao, porem, e verificada apenas pela coordenada x
 *   armazenada em espinhos[].x — a segunda coluna ocupada pelo
 *   emoji e puramente visual e nao interfere na logica do jogo.
 *   Isso significa que visualmente o espinho "escorrega" 1 coluna
 *   para a direita, o que e aceitavel e esperado com emojis.
 */
void desenharEspinho(int x, int y) {
    setColor(COR_VERMELHO);
    gotoxy(x, y);
    printf(THORN_EMOJI);  /* usa a constante definida no bloco de emojis */
    setColor(COR_RESET);
}

/*
 * gerarEspinho()
 * Posiciona o espinho [idx] em uma celula livre aleatoria.
 * Evita sobrepor cobras, frutas e outros espinhos.
 * Tambem evita as bordas imediatas (margem de 2 celulas)
 * para nao bloquear caminhos logo de cara.
 */
void gerarEspinho(int idx) {
    int x, y;
    do {
        x = rand() % (WIDTH - 4) + 3;
        y = rand() % (HEIGHT - 4) + TOP_OFFSET + 3;
    } while (
        cobraOcupaPosicao(x, y)  ||
        frutaOcupaPosicao(x, y)  ||
        espinhoOcupaPosicao(x, y)
    );
    espinhos[idx].x = x;
    espinhos[idx].y = y;
    desenharEspinho(x, y);
}

/*
 * iniciarEspinhos()
 * Define quantos espinhos existem com base na dificuldade e
 * chama gerarEspinho() para cada um.
 *
 *   Facil   -> 0 espinhos (jogo classico sem obstaculos)
 *   Normal  -> ESPINHOS_NORMAL  (4 fixos, nao mudam de lugar)
 *   Dificil -> ESPINHOS_DIFICIL (8; alguns reposicionam ao comer fruta)
 */
void iniciarEspinhos() {
    if      (velocidade == FACIL)   numEspinhos = 0;
    else if (velocidade == NORMAL)  numEspinhos = ESPINHOS_NORMAL;
    else                            numEspinhos = ESPINHOS_DIFICIL;

    for (int i = 0; i < numEspinhos; i++) gerarEspinho(i);
}

/*
 * reposicionarEspinhosDificil()
 * No modo Dificil, chamada toda vez que uma fruta e comida:
 * metade dos espinhos (os indices pares) mudam de lugar,
 * aumentando o desafio progressivamente.
 * A posicao antiga e apagada antes de redesenhar a nova.
 */
void reposicionarEspinhosDificil() {
    if (velocidade != DIFICIL) return;
    for (int i = 0; i < numEspinhos; i += 2) {
        /* Apaga o espinho antigo */
        gotoxy(espinhos[i].x, espinhos[i].y);
        printf(" ");
        /* Gera nova posicao e desenha */
        gerarEspinho(i);
    }
}

/*
 * redesenharEspinhos()
 * Reexibe todos os espinhos na tela (necessario apos system("cls")
 * ou apos a cobra passar por cima e apagar o caractere).
 */
void redesenharEspinhos() {
    for (int i = 0; i < numEspinhos; i++)
        desenharEspinho(espinhos[i].x, espinhos[i].y);
}

/* ================================================================
 *  FRUTAS
 * ================================================================ */

void gerarFruta(int idx) {
    int x, y;
    do {
        x = rand() % (WIDTH - 2) + 2;
        y = rand() % (HEIGHT - 2) + TOP_OFFSET + 2;
    } while (
        cobraOcupaPosicao(x, y)  ||
        frutaOcupaPosicao(x, y)  ||
        espinhoOcupaPosicao(x, y)  /* fruta nao aparece sobre espinho */
    );
    frutas[idx].x    = x;
    frutas[idx].y    = y;
    frutas[idx].tipo = tipoFrutaSel;

    setColor(frutaCor[frutas[idx].tipo]);
    gotoxy(x, y);
    printf("%s", frutaEmoji[frutas[idx].tipo]);
    setColor(COR_RESET);
}

void iniciarFrutas() {
    for (int i = 0; i < FRUIT_COUNT; i++) gerarFruta(i);
}

/* ================================================================
 *  HUD (HEADS-UP DISPLAY)
 * ================================================================ */

void desenharHUD() {
    setColor(COR_CIANO);
    gotoxy(0, 0);
    printf(" SNAKE GAME v2");

    /* Indicador de dificuldade com icone de espinho */
    setColor(velocidade == DIFICIL ? COR_VM_BRILHO :
             velocidade == NORMAL  ? COR_AMARELO : COR_VERDE);
    gotoxy(WIDTH - 14, 0);
    printf("[%s | ^x%d]",
        velocidade == FACIL ? "FACIL" : velocidade == DIFICIL ? "DIFICIL" : "NORMAL",
        numEspinhos);

    setColor(COR_VERDE);
    gotoxy(0, 1);
    printf(" P1[%s]: %04d", jogador[0].nome, jogador[0].score);

    if (modo == DOIS_JOGADORES) {
        setColor(COR_AMARELO);
        gotoxy(WIDTH / 2, 1);
        printf(" P2[%s]: %04d", jogador[1].nome, jogador[1].score);
    }

    setColor(COR_CINZA);
    gotoxy(0, 2);
    printf(" [P]=Pausar  [ESC]=Sair");

    setColor(COR_RESET);
}

/* ================================================================
 *  BORDA DO CAMPO
 * ================================================================ */

/*
 * desenharBorda()
 * Desenha as paredes do campo usando o emoji WALL_EMOJI (quadrado preto).
 *
 * IMPLEMENTACAO DE EMOJI NA BORDA:
 *   O emoji WALL_EMOJI ocupa 2 colunas no terminal (largura dupla).
 *   Por isso, ao inves de imprimir caractere por caractere em colunas
 *   consecutivas, usamos gotoxy() avancando de 2 em 2:
 *     for (col = 0; col <= WIDTH; col += 2) { gotoxy(col, y); printf(WALL_EMOJI); }
 *   Isso evita que um emoji sobreponha o inicio do proximo.
 *
 *   As linhas verticais tambem usam gotoxy(col, y) com col fixo
 *   na coluna 0 (esquerda) e WIDTH (direita), e o emoji preenche
 *   naturalmente as 2 colunas sem desalinhar o campo interno.
 */
void desenharBorda() {
    /* Largura efetiva da borda em colunas reais do terminal.
     * Como cada WALL_EMOJI ocupa 2 colunas, a borda superior/inferior
     * vai de col=0 ate col=WIDTH, pulando de 2 em 2. */
    int lin_inf = TOP_OFFSET + HEIGHT + 1;

    setColor(COR_CIANO);

    /* Linha superior — emoji por emoji, de 2 em 2 colunas */
    for (int col = 0; col <= WIDTH; col += 2) {
        gotoxy(col, TOP_OFFSET);
        printf(WALL_EMOJI);
    }

    /* Linha inferior — mesmo esquema */
    for (int col = 0; col <= WIDTH; col += 2) {
        gotoxy(col, lin_inf);
        printf(WALL_EMOJI);
    }

    /* Laterais — coluna 0 (esquerda) e coluna WIDTH (direita) */
    for (int y = TOP_OFFSET + 1; y < lin_inf; y++) {
        gotoxy(0,     y); printf(WALL_EMOJI);  /* parede esquerda */
        gotoxy(WIDTH, y); printf(WALL_EMOJI);  /* parede direita  */
    }

    setColor(COR_RESET);
}

/* ================================================================
 *  MOVIMENTO DA COBRA
 *  Array de segmentos: corpo[0] = cabeca, corpo[length-1] = cauda.
 *  Para mover, desloca-se todo o array uma posicao e atualiza [0].
 * ================================================================ */

void moverCobra(Cobra *c, int crescer) {
    for (int i = c->length - 1; i > 0; i--)
        c->corpo[i] = c->corpo[i - 1];

    int dx = (c->dir == DIREITA) - (c->dir == ESQUERDA);
    int dy = (c->dir == BAIXO)   - (c->dir == CIMA);
    c->corpo[0].x += dx;
    c->corpo[0].y += dy;

    if (crescer && c->length < MAX_TAIL) c->length++;
}

/* ================================================================
 *  DETECCAO DE COLISAO
 * ================================================================ */

/*
 * checarColisao()
 * Verifica se a cabeca do jogador [p] colidiu com:
 *   1. Paredes do campo
 *   2. Proprio corpo
 *   3. Corpo do outro jogador (modo 2P)
 *   4. Espinhos (NOVO — mata igual a parede)
 */
int checarColisao(int p) {
    int hx = jogador[p].corpo[0].x;
    int hy = jogador[p].corpo[0].y;

    /* Parede */
    if (hx <= 0 || hx > WIDTH || hy <= TOP_OFFSET || hy > TOP_OFFSET + HEIGHT)
        return 1;

    /* Proprio corpo */
    for (int i = 1; i < jogador[p].length; i++)
        if (jogador[p].corpo[i].x == hx && jogador[p].corpo[i].y == hy)
            return 1;

    /* Outro jogador (modo 2P) */
    int outro = 1 - p;
    if (modo == DOIS_JOGADORES && jogador[outro].vivo)
        for (int i = 0; i < jogador[outro].length; i++)
            if (jogador[outro].corpo[i].x == hx && jogador[outro].corpo[i].y == hy)
                return 1;

    /*
     * Espinho — colisao fatal igual a parede.
     * O espinho e desenhado como "^"; se a cabeca chegar na mesma
     * celula, o jogador morre imediatamente.
     */
    if (espinhoOcupaPosicao(hx, hy)) return 1;

    return 0;
}

/* ================================================================
 *  DESENHO DA COBRA
 *  Otimizacao: redesenha apenas cabeca e apaga cauda antiga,
 *  evitando piscar ao redesenhar o corpo inteiro todo frame.
 * ================================================================ */

void desenharCobra(int p, Ponto caudaAntiga) {
    Cobra *c = &jogador[p];

    gotoxy(caudaAntiga.x, caudaAntiga.y);
    printf(" ");

    /*
     * ATENCAO: se um espinho estiver na mesma linha que a cauda antiga,
     * o printf(" ") acima pode apaga-lo visualmente. Por isso chamamos
     * redesenharEspinhos() apos o loop principal de atualizacao.
     * Isso e necessario porque a cobra pode passar por cima do caractere
     * do espinho enquanto o espinho ainda esta ativo na logica do jogo.
     */

    if (c->length > 1) {
        setColor(c->cor);
        gotoxy(c->corpo[1].x, c->corpo[1].y);
        printf("o");
    }

    setColor(p == 0 ? COR_BRANCO : COR_AMARELO);
    gotoxy(c->corpo[0].x, c->corpo[0].y);
    printf(p == 0 ? "O" : "0");
    setColor(COR_RESET);
}

/* ================================================================
 *  INICIALIZACAO DO JOGO
 * ================================================================ */

void iniciarJogo() {
    system("cls");

    jogador[0].length = 3; jogador[0].dir = DIREITA;
    jogador[0].score  = 0; jogador[0].vivo = 1; jogador[0].cor = COR_VERDE;
    for (int i = 0; i < 3; i++) {
        jogador[0].corpo[i].x = WIDTH / 3 - i;
        jogador[0].corpo[i].y = TOP_OFFSET + HEIGHT / 2;
    }

    jogador[1].length = 3; jogador[1].dir = ESQUERDA;
    jogador[1].score  = 0;
    jogador[1].vivo   = (modo == DOIS_JOGADORES) ? 1 : 0;
    jogador[1].cor    = COR_AMARELO;
    for (int i = 0; i < 3; i++) {
        jogador[1].corpo[i].x = (WIDTH / 3) * 2 + i;
        jogador[1].corpo[i].y = TOP_OFFSET + HEIGHT / 2;
    }

    desenharBorda();
    desenharHUD();
    iniciarFrutas();
    iniciarEspinhos();   /* <-- gera os espinhos apos frutas e cobras */
    estado = JOGANDO;
}

/* ================================================================
 *  LEITURA DE INPUT
 * ================================================================ */

void lerInput() {
    while (_kbhit()) {
        int k = _getch();
        if (k == 27) { estado = FIM; return; }
        if (k == 'p' || k == 'P') {
            estado = (estado == PAUSADO) ? JOGANDO : PAUSADO;
            return;
        }
        if (k == 224 || k == 0) {
            int k2 = _getch();
            if (k2 == 72 && jogador[0].dir != BAIXO)    jogador[0].dir = CIMA;
            if (k2 == 80 && jogador[0].dir != CIMA)     jogador[0].dir = BAIXO;
            if (k2 == 75 && jogador[0].dir != DIREITA)  jogador[0].dir = ESQUERDA;
            if (k2 == 77 && jogador[0].dir != ESQUERDA) jogador[0].dir = DIREITA;
        }
        if (modo == DOIS_JOGADORES && jogador[1].vivo) {
            if ((k=='w'||k=='W') && jogador[1].dir != BAIXO)    jogador[1].dir = CIMA;
            if ((k=='s'||k=='S') && jogador[1].dir != CIMA)     jogador[1].dir = BAIXO;
            if ((k=='a'||k=='A') && jogador[1].dir != DIREITA)  jogador[1].dir = ESQUERDA;
            if ((k=='d'||k=='D') && jogador[1].dir != ESQUERDA) jogador[1].dir = DIREITA;
        }
    }
}

/* ================================================================
 *  LOOP DE ATUALIZACAO (1 FRAME)
 * ================================================================ */

void atualizarJogo() {
    int precisaRedesenharEspinhos = 0;

    for (int p = 0; p < 2; p++) {
        if (!jogador[p].vivo || jogador[p].dir == PARADO) continue;

        Ponto caudaAntiga = jogador[p].corpo[jogador[p].length - 1];

        /* Calcula proxima posicao para verificar fruta */
        int dx = (jogador[p].dir == DIREITA) - (jogador[p].dir == ESQUERDA);
        int dy = (jogador[p].dir == BAIXO)   - (jogador[p].dir == CIMA);
        int nx = jogador[p].corpo[0].x + dx;
        int ny = jogador[p].corpo[0].y + dy;

        int comeuFruta = 0;
        for (int i = 0; i < FRUIT_COUNT; i++) {
            if (frutas[i].x == nx && frutas[i].y == ny) {
                comeuFruta = 1;
                jogador[p].score += 10 + (velocidade == DIFICIL ? 10 :
                                          velocidade == NORMAL   ?  5 : 0);
                gerarFruta(i);
                /* No modo dificil, parte dos espinhos muda de lugar */
                if (velocidade == DIFICIL) {
                    reposicionarEspinhosDificil();
                    precisaRedesenharEspinhos = 1;
                }
                break;
            }
        }

        moverCobra(&jogador[p], comeuFruta);

        if (checarColisao(p)) {
            jogador[p].vivo = 0;
            setColor(COR_VM_BRILHO);
            gotoxy(jogador[p].corpo[0].x, jogador[p].corpo[0].y);
            printf("X");
            setColor(COR_RESET);
            sleep_ms(200);
            continue;
        }

        desenharCobra(p, caudaAntiga);
    }

    /*
     * Redesenha espinhos apagados acidentalmente pelo movimento das cobras.
     * Necessario porque desenharCobra() imprime " " na posicao da cauda,
     * podendo sobrescrever o "^" de um espinho proximo.
     */
    if (precisaRedesenharEspinhos) redesenharEspinhos();

    /* Garantia extra: re-exibe espinhos todo frame (custo minimo) */
    redesenharEspinhos();

    desenharHUD();

    int vivos = jogador[0].vivo + jogador[1].vivo;
    if (modo == UM_JOGADOR  && !jogador[0].vivo) estado = FIM;
    if (modo == DOIS_JOGADORES && vivos == 0)    estado = FIM;
}

/* ================================================================
 *  TELAS
 * ================================================================ */

void desenharLogoASCII() {
    const char *logo[] = {
        "  _____ _   _    _    _  _______ ",
        " / ____| \\ | |  / \\  | |/ / ____|",
        "| (___ |  \\| | / _ \\ | ' /|  _|  ",
        " \\___ \\| . ` |/ ___ \\| . \\| |___ ",
        " ____) | |\\  / / ___ \\ |\\  |  __|",
        "|_____/|_| \\_/_/   \\_\\_| \\_|_____|",
        "            G A M E v2        "
    };
    int cores[] = { COR_VERDE, COR_VERDE, COR_CIANO, COR_CIANO, COR_AZUL, COR_AZUL, COR_AMARELO };
    for (int i = 0; i < 7; i++)
        centralizarTexto(2 + i, 60, logo[i], cores[i]);
}

int telaMenu() {
    system("cls");
    desenharLogoASCII();

    int l = 11;
    centralizarTexto(l++, 60, "================================", COR_CINZA);
    centralizarTexto(l++, 60, "  PRESSIONE ENTER PARA JOGAR  ",  COR_BRANCO);
    centralizarTexto(l++, 60, "================================", COR_CINZA);
    l++;
    centralizarTexto(l++, 60, "[M] Modo: UM JOGADOR / DOIS JOGADORES",   COR_CIANO);
    centralizarTexto(l++, 60, "[V] Velocidade: FACIL / NORMAL / DIFICIL", COR_VERDE);
    centralizarTexto(l++, 60, "[F] Fruta: (cicla entre as opcoes)",        COR_AMARELO);
    l++;

    char buf[64];
    sprintf(buf, "MODO: %s", modo == DOIS_JOGADORES ? "2 JOGADORES" : "1 JOGADOR");
    centralizarTexto(l++, 60, buf, COR_BRANCO);

    sprintf(buf, "VELOCIDADE: %s  (espinhos: %d)",
        velocidade == FACIL ? "FACIL" : velocidade == DIFICIL ? "DIFICIL" : "NORMAL",
        velocidade == FACIL ? 0 : velocidade == DIFICIL ? ESPINHOS_DIFICIL : ESPINHOS_NORMAL);
    centralizarTexto(l++, 60, buf, COR_BRANCO);

    sprintf(buf, "FRUTA: [%s] %s", frutaEmoji[tipoFrutaSel], frutaNome[tipoFrutaSel]);
    centralizarTexto(l++, 60, buf, frutaCor[tipoFrutaSel]);

    l++;
    /* Legenda de espinhos */
    centralizarTexto(l++, 60, "^ = espinho (mata ao tocar) | FACIL = sem espinhos", COR_VERMELHO);
    l++;
    centralizarTexto(l++, 60, "[R] Ver Ranking  |  [ESC] Sair", COR_CINZA);

    while (1) {
        int k = _getch();
        if (k == 13)           return 1;
        if (k == 27)           return 0;
        if (k=='r'||k=='R')    return 2;
        if (k=='m'||k=='M') { modo = (modo == UM_JOGADOR) ? DOIS_JOGADORES : UM_JOGADOR; return telaMenu(); }
        if (k=='v'||k=='V') {
            if      (velocidade == FACIL)  velocidade = NORMAL;
            else if (velocidade == NORMAL) velocidade = DIFICIL;
            else                           velocidade = FACIL;
            return telaMenu();
        }
        if (k=='f'||k=='F') { tipoFrutaSel = (tipoFrutaSel + 1) % 8; return telaMenu(); }
    }
}

int telaNomes() {
    system("cls");
    setColor(COR_CIANO); gotoxy(5, 5);
    printf("=== IDENTIFICACAO DOS JOGADORES ===");
    setColor(COR_BRANCO); gotoxy(5, 8);
    printf("Nome do Jogador 1 (P1): ");
    setColor(COR_VERDE);
    scanf("%20s", jogador[0].nome);
    if (modo == DOIS_JOGADORES) {
        setColor(COR_BRANCO); gotoxy(5, 10);
        printf("Nome do Jogador 2 (P2): ");
        setColor(COR_AMARELO);
        scanf("%20s", jogador[1].nome);
    } else {
        strcpy(jogador[1].nome, "CPU");
    }
    setColor(COR_CINZA); gotoxy(5, 13);
    printf("Pressione ENTER para comecar...");
    while (_getch() != 13);
    return 1;
}

void telaPausa() {
    int cx = (WIDTH / 2) - 10;
    int cy = TOP_OFFSET + HEIGHT / 2 - 1;
    setColor(COR_AMARELO);
    gotoxy(cx, cy);     printf("  ======================  ");
    gotoxy(cx, cy + 1); printf("     ||  PAUSADO  ||      ");
    gotoxy(cx, cy + 2); printf("  [P] para continuar      ");
    gotoxy(cx, cy + 3); printf("  ======================  ");
    setColor(COR_RESET);
}

int telaGameOver() {
    inserirNoRanking(jogador[0].nome, jogador[0].score, (int)modo);
    if (modo == DOIS_JOGADORES)
        inserirNoRanking(jogador[1].nome, jogador[1].score, (int)modo);

    sleep_ms(500);

    int bx = (WIDTH - 36) / 2;
    int by = TOP_OFFSET + 1;
    int bw = 40;
    int bh = 17;

    /* Fundo da caixa */
    for (int r = by; r <= by + bh; r++) {
        gotoxy(bx, r);
        for (int c = 0; c < bw; c++) printf(" ");
    }

    /* Borda vermelha */
    setColor(COR_VM_BRILHO);
    gotoxy(bx, by);
    printf("\xC9"); for (int i=1;i<bw-1;i++) printf("\xCD"); printf("\xBB");
    gotoxy(bx, by+bh);
    printf("\xC8"); for (int i=1;i<bw-1;i++) printf("\xCD"); printf("\xBC");
    for (int r = by+1; r < by+bh; r++) {
        gotoxy(bx, r);        printf("\xBA");
        gotoxy(bx+bw-1, r);   printf("\xBA");
    }

    /* Titulo */
    setColor(COR_VM_BRILHO);
    gotoxy(bx + 9, by + 1); printf("**** GAME  OVER ****");

    /* Separador */
    setColor(COR_CINZA);
    gotoxy(bx+1, by+2); for (int i=0;i<bw-2;i++) printf("-");

    /* Pontuacoes */
    setColor(COR_VERDE);
    gotoxy(bx+3, by+4);
    printf("%-10s : %04d pts", jogador[0].nome, jogador[0].score);

    if (modo == DOIS_JOGADORES) {
        setColor(COR_AMARELO);
        gotoxy(bx+3, by+5);
        printf("%-10s : %04d pts", jogador[1].nome, jogador[1].score);
        setColor(COR_CIANO);
        gotoxy(bx+3, by+7);
        if      (jogador[0].score > jogador[1].score) printf(">> Vencedor: %s! <<", jogador[0].nome);
        else if (jogador[1].score > jogador[0].score) printf(">> Vencedor: %s! <<", jogador[1].nome);
        else                                           printf(">> EMPATE! <<");
    } else {
        setColor(COR_CINZA);
        gotoxy(bx+3, by+6);
        printf("Voce colidiu e morreu!");
        /* Informa se foi em espinho */
        int hx = jogador[0].corpo[0].x, hy = jogador[0].corpo[0].y;
        if (espinhoOcupaPosicao(hx, hy)) {
            setColor(COR_VERMELHO);
            gotoxy(bx+3, by+7);
            printf("(bateu em um espinho!)");
        }
    }

    /* Separador ranking */
    setColor(COR_CINZA);
    gotoxy(bx+1, by+9); for (int i=0;i<bw-2;i++) printf("-");

    setColor(COR_AMARELO);
    gotoxy(bx+3, by+10); printf("=== TOP %d RANKING ===", MAX_RANKING);

    EntradaRanking ranking[MAX_RANKING];
    int n = carregarRanking(ranking);
    const char *medalhas[] = {"1o", "2o", "3o", "4o", "5o"};
    int cores_r[] = { COR_AMARELO, COR_CINZA, COR_DOURADO, COR_RESET, COR_RESET };
    for (int i = 0; i < n && i < MAX_RANKING; i++) {
        setColor(cores_r[i]);
        gotoxy(bx+3, by+11+i);
        printf("%s %-10s %04d [%dP]",
               medalhas[i], ranking[i].nome, ranking[i].score, ranking[i].modo);
    }

    setColor(COR_VERDE);
    gotoxy(bx+3, by+bh-1);
    printf("[ENTER]=Jogar Novamente  [ESC]=Menu");
    setColor(COR_RESET);

    while (1) {
        int k = _getch();
        if (k == 13) return 1;
        if (k == 27) return 0;
    }
}

void telaRanking() {
    system("cls");
    centralizarTexto(2, 60, "=== HALL OF FAME - TOP 5 ===", COR_AMARELO);
    EntradaRanking r[MAX_RANKING];
    int n = carregarRanking(r);
    if (n == 0) {
        centralizarTexto(6, 60, "Nenhum recorde salvo ainda!", COR_CINZA);
    } else {
        const char *m[] = { "[1o]","[2o]","[3o]","[4o]","[5o]" };
        int  c[] = { COR_AMARELO, COR_CINZA, COR_DOURADO, COR_RESET, COR_RESET };
        for (int i = 0; i < n; i++) {
            char buf[64];
            sprintf(buf, "%s  %-15s  %04d pts  [%d jogador(es)]",
                    m[i], r[i].nome, r[i].score, r[i].modo);
            centralizarTexto(5 + i*2, 60, buf, c[i]);
        }
    }
    centralizarTexto(18, 60, "Pressione qualquer tecla para voltar...", COR_CINZA);
    _getch();
}

/* ================================================================
 *  MAIN — MAQUINA DE ESTADOS
 *
 *  Estados:
 *    MENU    -> telaMenu()   : seleciona opcoes
 *    SELECAO -> telaNomes()  : digita nomes, inicia jogo
 *    JOGANDO -> loop rapido  : input -> update -> sleep
 *    PAUSADO -> telaPausa()  : aguarda P para continuar
 *    FIM     -> telaGameOver(): exibe resultado e ranking
 * ================================================================ */
int main() {
    /*
     * ATIVACAO DO UTF-8 NO CONSOLE
     * Para que os emojis aparecam corretamente no Windows,
     * ativamos UTF-8 de duas formas complementares:
     *   system("chcp 65001") — muda o code page via comando do sistema
     *   SetConsoleOutputCP(65001) — muda diretamente via WinAPI
     *   setlocale(LC_ALL, ".UTF-8") — configura o locale do programa
     * Usar as tres garante compatibilidade maxima entre versoes do Windows.
     */
    system("chcp 65001 > nul");       /* Ativa UTF-8 para emojis (cmd)     */
    SetConsoleOutputCP(65001);        /* Ativa UTF-8 via WinAPI             */
    setlocale(LC_ALL, ".UTF-8");      /* Locale UTF-8 do programa           */
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    srand((unsigned)time(NULL));
    hideCursor();
    system("mode con: cols=70 lines=32");
    system("title Snake Game v2");
    system("cls");

    int rodando = 1;
    while (rodando) {

        if (estado == MENU) {
            int res = telaMenu();
            if (res == 0) { rodando = 0; break; }
            if (res == 2) { telaRanking(); continue; }
            estado = SELECAO;
        }

        if (estado == SELECAO) {
            if (!telaNomes()) { estado = MENU; continue; }
            iniciarJogo();
        }

        while (estado == JOGANDO || estado == PAUSADO) {
            lerInput();
            if (estado == PAUSADO) { telaPausa(); sleep_ms(100); continue; }
            atualizarJogo();
            sleep_ms((int)velocidade);
        }

        if (estado == FIM) {
            int res = telaGameOver();
            estado = (res == 1) ? SELECAO : MENU;
        }
    }

    system("cls");
    setColor(COR_VERDE);
    printf("\n  Obrigado por jogar Snake Game!\n\n");
    setColor(COR_RESET);
    return 0;
}
