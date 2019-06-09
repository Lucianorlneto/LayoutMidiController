#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/aruco.hpp>
#include <math.h>
#include <iostream>
#include <zbar.h>
#include <highgui.h> 
#include "midimessages.h"
#include <rtmidi/RtMidi.h>
#include <unistd.h>
#include <cstdlib>
#include <time.h>
#include <string>

using namespace cv;
using namespace std;
using namespace zbar;

//VARIÁVEIS DE MIDI
RtMidiOut *midiout;
vector<unsigned char> message;
/////////////////////////////////////////

//VARIÁVEIS PARA TRANSFORMAÇÃO ESPACIAL
Point2f src_vertices[4], dst_vertices[4];
/////////////////////////////////////////

//VARIÁVEIS PARA IMAGENS
//imagem principal
Mat src;
//matriz copia da original para sinalizações
Mat image;
//matriz para detecção de marcação no grid
Mat celula;
//matriz para detecção de marcação no grid filtrada
Mat mask1;
//matriz para transformada espacial
Mat warpPerspectiveMatrix;
//matriz para resultado da transformada espacial
Mat destinationImage(500,600,CV_8UC3);
Mat destinationImage2;
/////////////////////////////////////////

//VARIAVÉIS DE CONTROLE
//controle dos comandos
int keyEx;
//código de barras codificado, stop on, noteon on
bool decodificado, stop, noteon;
//numero de cigos de barra detectados
int n_objetos;
//controle de oitava
int oitava;
//controle de canal da bateria
bool bateriaOn;
//bumbo, caixa, surdo, tom, tom, tom, symbal, symbal, symbal, ataque, condução, china
int bateria_notas[13] = {54, 53, 58, 51, 48, 46, 44, 52, 50, 49, 43, 40, 47};
//string do código de barras
string linhas_barcode_str = "";
string barcode_str2 = "";
string barcode_str3 = "";
int linhas_barcode_int = 0;
int barcode_int2 = 0;
int barcode_int3 = 0;
//vetor de ntoas anterioes que devem ser desligadas
vector<int> notas_anteriores;
//notas para serem mantidas ligadas
vector<int> notas_atuais;
/////////////////////////////////////////

bool tocada[200];

//VARIÁVEIS ARUCO
enum{ZERO, TOPLEFT, TOPRIGHT, BOTTOMLEFT, BOTTOMRIGHT};
vector<int> ids;
vector< vector<Point2f> > markerCorners, rejectedCandidates;
cv::Ptr<cv::aruco::DetectorParameters> parameters;
cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_250);
/////////////////////////////////////////


//struct para código de barras
typedef struct
{
  string type;
  string data;
  vector <Point> location;
} decodedObject;
/////////////////////////////////////////

void find_marker(){
    parameters = aruco::DetectorParameters::create();
    cv::aruco::detectMarkers(src, dictionary, markerCorners, ids, parameters, rejectedCandidates);
    for(int i=0; i<(int)ids.size(); i++){
        switch(ids[i]){
            case TOPLEFT:
                circle(image, markerCorners[i][3], 3, Scalar(255,0,0), 3);
                src_vertices[0] = markerCorners[i][3];
                break;
            case TOPRIGHT:
                circle(image, markerCorners[i][2], 3, Scalar(0,255,0), 3);
                src_vertices[1] = markerCorners[i][2];
                break;
            case BOTTOMLEFT:
                circle(image, markerCorners[i][0], 3, Scalar(0,0,255), 3);
                src_vertices[3] = markerCorners[i][0];
                break;
            case BOTTOMRIGHT:
                circle(image, markerCorners[i][1], 3, Scalar(255,0,255), 3);
                src_vertices[2] = markerCorners[i][1];
                break;
        }
    }
}

// Find and decode barcodes and QR codes
void decode(Mat &im, vector<decodedObject>&decodedObjects)
{
   
    // Create zbar scanner
    ImageScanner scanner;
 
    // Configure scanner
    scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
   
    // Convert image to grayscale
    Mat imGray;
    cvtColor(im, imGray,CV_BGR2GRAY);
 
    // Wrap image data in a zbar image
    Image image(im.cols, im.rows, "Y800", (uchar *)imGray.data, im.cols * im.rows);
 
    // Scan the image for barcodes and QRCodes
    n_objetos = scanner.scan(image);
    if(n_objetos > 0){
        decodificado = true;
    }
   
    // Print results
    for(Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol)
    {
        decodedObject obj;
     
        obj.type = symbol->get_type_name();
        obj.data = symbol->get_data();
     
        // Print type and data
        // cout << "Type : " << obj.type << endl;
        // cout << "Data : " << obj.data << endl << endl;
     
        // Obtain location
        for(int i = 0; i< symbol->get_location_size(); i++)
        {
            obj.location.push_back(Point(symbol->get_location_x(i),symbol->get_location_y(i)));
        }
     
        decodedObjects.push_back(obj);
    }
}

//show decodedObject
void display(Mat &im, vector<decodedObject>&decodedObjects)
{
    // Loop over all decoded objects
    for(int i = 0; i < (int)decodedObjects.size(); i++)
    {
        vector<Point> points = decodedObjects[i].location;
        vector<Point> hull;
     
        // If the points do not form a quad, find convex hull
        if(points.size() > 4)
          convexHull(points, hull);
        else
          hull = points;
         
        // Number of points in the convex hull
        int n = hull.size();
         
        for(int j = 0; j < n; j++)
        {
          line(im, hull[j], hull[ (j+1) % n], Scalar(255,0,0), 3);
        }
         
    }
   
}

//Desliga todas as notas
void shutNotes(){
    for (int i = 0; i < 13; ++i)
    {
        message[0] = MIDI_NOTEOFF;
        message[1] = oitava*20+12-i;
        midiout->sendMessage( &message );
        noteon = false;
        notas_anteriores.clear();
    }
    for (int i = 0; i < 200; ++i)
    {
        tocada[i] = false;
    }
}

int main(int argc, char** argv)
{
    
    if(argc < 4){
        cout << endl << "COMO PARÂMETROS DE ENTRADA, INFORME O ID DA FONTE DE VÍDEO, O NÚMERO DE BATIDAS POR MINUTO (bpm) E A OITAVA QUE DESEJA, NESSA ORDEM." << endl;
        cout << "Ex.: ./LayouMidiController 0 280 3." << endl << endl;
        return 0;
    }

    cout << "VIDEO SOURCE: " << argv[1] << endl;
    cout << "BATIDAS POR MINUTO: " << argv[2] << endl;
    cout << "OITAVA: " << argv[3] << endl << endl;
    // cout << "OpenCV version : " << CV_VERSION << endl;

    //STOP
    stop = true;
    /////////////////////////////////

    //VARIÁVEL PARA SABER SE O CÓDIGO DE BARRAS JÁ FOI DECODIFICADO
    decodificado = false;
    /////////////////////////////////

    //INICIALIZAÇÃO MIDI
    unsigned int nPorts, portOut;
    string portName;

    // RtMidiOut constructor
    try {
        midiout = new RtMidiOut();
    }
    catch ( RtMidiError &error ) {
        error.printMessage();
        exit( EXIT_FAILURE );
    }
    // Check outputs.
    nPorts = midiout->getPortCount();
    cout << "Existem " << nPorts << " MIDI de entrada disponíveis.\n";
    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
          portName = midiout->getPortName(i);
      }
      catch (RtMidiError &error) {
          error.printMessage();
          waitKey();
      }
      cout << "Porta #" << i << ": " << portName << '\n';
    }
    cout << "Escolha a porta para conexão: ";
    cin >> portOut;
    // Open first available port.
    midiout->openPort( portOut );
    // Send out a series of MIDI messages.
    // Program change: 192, 5
    message.push_back( 192 );
    message.push_back( 0 );


    midiout->sendMessage( &message );
    // Control Change: 176, 7, 100 (volume)
    message[0] = 176;
    message[1] = 7;
    message.push_back( 100 );
    midiout->sendMessage( &message );

    //CÁLCULOS DE TEMPO (BATIDAS POR SEGUNDO)
    string bpm_string = argv[2];
    int bpm = 0;
    bpm = stoi(bpm_string);
    float tempo = 60.0/(float)bpm;
    float tempo_linha = 0;
    tempo_linha = (((tempo)*1000000));
    string oitava_char = argv[3];
    oitava = 0;
    oitava = stoi(oitava_char);
    ////////////////////////////////////
    bateriaOn = false;

    //ABRIR CAMERA
    VideoCapture cap(stoi(argv[1]));
    cap >> src;
    /////////////////////////////////////

    //VETOR DE CÓDIGOS DE BARRA
    vector<decodedObject> decodedObjects;
    ////////////////////////////////////

    //inicializando vetor de tocadas
    for (int i = 0; i < 200; ++i)
    {
        tocada[i] = false;
    }
    /////////////////////////////////////

    cout << endl << "CONTROLES:" << endl << endl;
    cout << "EM STOP:" << endl;
    cout << "SETA PARA CIMA - AUMENTA VELOCIDADE" << endl;
    cout << "SETA PARA BAIXO - DIMINUI VELOCIDADE" << endl;
    cout << "SETA PARA DIREITA - AUMENTA OITAVA" << endl;
    cout << "SETA PARA ESQUERDA - DIMINUI OITAVA" << endl;
    cout << "BARRA DE ESPAÇO - PLAY" << endl;
    cout << "q ou ESC para finalizar" << endl;
    cout << "m para alternar entre modo de bateria" << endl << endl;
    cout << "EM PLAY:" << endl;
    cout << "BARRA DE ESPAÇO - STOP" << endl;
    cout << "q ou ESC para finalizar" << endl << endl;

    //LOOP PRINCIPAL
    inicio:
    // cout << "começou" << endl;
    while(1){
        noteon = false;

        cap >> src;

        //COPIA FRAME PARA JANELA AUXILIAR DE INDICAÇÕES
        src.copyTo(image);

        //PROCURA MARCADORES ARUCO
        find_marker();

        //VERIFICA SE JÁ FORAM FEITOS OS 4 QUADRADOS COM AS CORES
        if(ids.size() == 4){
            //4 MARCADORES ARUCO IDENTIFICADOS;

            if(stop == true){
                //SE O CÓDIGO DE BARRAS NÃO ESTIVER DECODIFICADO, DECODIFICÁ-LO
                if(decodificado == false){
                    decode(src, decodedObjects);
                    if(n_objetos > 0){
                        // cout << decodedObjects[0].data << endl;
                        linhas_barcode_str = decodedObjects[0].data[0];
                        linhas_barcode_str = linhas_barcode_str + decodedObjects[0].data[1];
                        // cout << linhas_barcode_str << endl;
                        linhas_barcode_int = stoi(linhas_barcode_str);

                        barcode_str2 = decodedObjects[0].data[2];
                        barcode_str2 = barcode_str2 + decodedObjects[0].data[3];
                        // cout << barcode_str2 << endl;
                        barcode_int2 = stoi(barcode_str2);

                        barcode_str3 = decodedObjects[0].data[4];
                        barcode_str3 = barcode_str3 + decodedObjects[0].data[5];
                        // cout << barcode_str3 << endl;
                        barcode_int3 = stoi(barcode_str3);
                    }
                }

                //VERIFICA SE O CÓDIGO DE BARRAS FOI INTERPRETADO
                if(decodificado == true){
                    //DESENHA APONTADOR DO CÓDIGO DE BARRAS
                    display(image, decodedObjects);

                    //TRANSFORMADA DO GRID
                    dst_vertices[0] = Point(0,0);
                    dst_vertices[1] = Point(linhas_barcode_int*30-2,0);
                    dst_vertices[2] = Point(linhas_barcode_int*30-2,13*30-1);
                    dst_vertices[3] = Point(0,13*30-1);

                    warpPerspectiveMatrix = getPerspectiveTransform(src_vertices, dst_vertices);

                    warpPerspective(src, destinationImage, warpPerspectiveMatrix, Size(linhas_barcode_int*30-2,13*30-1), INTER_LINEAR, BORDER_CONSTANT);

                    imshow("Transformada",destinationImage);
                    //////////////////////////////
                }
            }else{
                //IDENTIFICA HORA DE COMEÇAR A TOCAR
                int i = 0, j = 0;
                bool azul = false;
                string nota = "";
                shutNotes();
                imshow("Transformada",destinationImage);
                tempo = 60.0/(float)bpm;
                tempo_linha = (tempo*1000000);
                // cout << barcode_int3 << endl;
                tempo_linha = tempo_linha/(float)barcode_int3;
                //copia para desenhar marcações da imagem transformada
                while(stop == false){
                    if(bateriaOn == false){
                        destinationImage.copyTo(destinationImage2);
                        for (i = 0; i < linhas_barcode_int; ++i)
                        {
                            //SE TIVER ALGUMA NOTA LIGADA E ELA ESTIVER NO VETOR DE NOTAS ANTERIOES, DESLIGÁ-LA
                            if(noteon == true){
                                for (int i = 0; i < (int)notas_anteriores.size(); ++i)
                                {
                                    message[0] = MIDI_NOTEOFF;
                                    message[1] = notas_anteriores[i];
                                    midiout->sendMessage( &message );
                                    noteon = false;
                                    tocada[notas_anteriores[i]] = false;
                                    // cout << "apagou nota " << notas_anteriores[i] << endl;
                                    notas_anteriores.erase(notas_anteriores.begin() + i);
                                }
                                // notas_anteriores.clear();
                            }

                            //SEPARAR GRID TRANSFORMADO EM POSIÇÕES PARA NOTAS
                            for (j = 0; j < 13; ++j)
                            {
                                //PRÓXIMA CÉLULA DO GRID NA ORDEM
                                celula = destinationImage(Rect((int)i*((float)destinationImage.rows/13), (int)(j*((float)destinationImage.cols/linhas_barcode_int)), (int)((float)destinationImage.rows/13), (int)((float)destinationImage.cols/linhas_barcode_int)));

                                //DESENHAR CÉLULAS LIDAS
                                rectangle(destinationImage2, Point(i*((float)destinationImage.rows/13) ,j*((float)destinationImage.cols/linhas_barcode_int)), Point((i+1)*((float)destinationImage.rows/13),(j+1)*((float)destinationImage.cols/linhas_barcode_int)), Scalar(0,255,0), 1);
                                imshow("Transformada",destinationImage2);


                                //FILTRAR O BRANCO DA CÉLULA
                                inRange(celula, Scalar(130, 130, 130), Scalar(255, 255, 255), mask1);

                                //POSIÇÕES PARA IDENTIFICAR SE HÁ ALGO DIFERENTE DE BRANCO NO CENTRO DA CÉLULA
                                for (int y = 11; y < mask1.rows-11; ++y)
                                {
                                    for (int x = 11; x < mask1.cols-11; ++x)
                                    {
                                        //VERIFICA SE O PIXEL ATUAL É PRETO
                                        Scalar colour = mask1.at<uchar>(Point(x, y));
                                        if(colour.val[0]==0){
                                            // cout << "nota " << oitava*20+12-j << endl;
                                            //SE O PIXEL FOR PRETO, LIGAR NOTA
                                            azul = true;

                                            //IDENTIFICAR SE A NOTA TOCANDO NO MOMENTO SERÁ DESLIGADA
                                            bool tem_nota = false;
                                            for (int a = 0; a < (int)notas_anteriores.size(); ++a)
                                            {
                                                if(notas_anteriores[a] == oitava*20+12-j){
                                                    tem_nota = true;
                                                }
                                            }

                                            //SE SIM, ELA PODE SER TOCADA NOVAMENTE
                                            if(tem_nota == true){
                                                // cout << "tem nota true" << endl;
                                                // cout << "tocou nota (seria apagada)" << oitava*20+12-j << endl;
                                                message[0] = MIDI_NOTEON;
                                                message[1] = oitava*20+12-j;
                                                midiout->sendMessage( &message );
                                                noteon = true; 
                                                tocada[oitava*20+12-j] = true;
                                                // tem_nota = false;
                                            }else{
                                                //SENÃO, CASO ELA AINDA NÃO TENHA SIDO TOCADA, TOCÁ-LA
                                                // cout << "tem nota false" << endl;
                                                if(tocada[oitava*20+12-j] == false){
                                                    // cout << "tocou nota" << endl; 
                                                    // cout << "tocou nota (mantem ligada)" << oitava*20+12-j << endl;   
                                                    message[0] = MIDI_NOTEON;
                                                    message[1] = oitava*20+12-j;
                                                    midiout->sendMessage( &message );
                                                    noteon = true; 
                                                    tocada[oitava*20+12-j] = true;
                                                }
                                                //SE A NOTA FOR LIGADA E NÃO ESTIVER NO ARRAY PARA DELISGAR, CONTINUA LIGADA
                                            }

                                            //SE OS PRÓXIMOS PIXELS DA DIREITA TAMBÉM FOREM PRETOS, NOTA NÃO SERÁ DESLIGADA A FORÇA
                                            for (int x1 = x; x1 < mask1.cols; ++x1)
                                            {
                                                Scalar colour2 = mask1.at<uchar>(Point(x1, 17));
                                                // imshow("mask1", mask1);
                                                // waitKey();
                                                // cout << colour2.val[0] << endl;
                                                // mask1.at<uchar>(Point(x1, 17)) = 0;
                                                //AO IDENTIFICAR PRÓXIMO PIXEL BRANCO, DESLIGAR NOTA NA ETAPA DE DESLIGAR AS NOTAS
                                                if(colour2.val[0]!=0){
                                                    // cout << "achou branco para a nota " << oitava*20+12-j << endl;
                                                    notas_anteriores.push_back(oitava*20+12-j);
                                                    // tocada[oitava*20+12-j] = false;
                                                    break;
                                                }
                                            }
                                            // cout << "tamanho notas_anteriores " << notas_anteriores.size() << endl;
                                        }
                                        // imshow("mask1", mask1);
                                        // waitKey();
                                        // mask1.at<uchar>(Point(x, y)) = 0;

                                        //suavização da quebra de loop
                                        if(azul == true){
                                            x = mask1.cols;
                                            y = mask1.rows;
                                            azul = false;
                                        }
                                    }
                                }
                            }
                            // decodedObjects.clear();
                            switch(waitKey(1)){
                                case 27:
                                    goto final;
                                case 32:
                                    if(!stop){
                                        stop = true;
                                        cout << "STOP" << endl;
                                        shutNotes();
                                        decodedObjects.clear();
                                        decodificado = false;
                                        goto inicio;
                                    }
                                case 'q':
                                    goto final;
                            }
                            // usleep(tempo_linha);
                            clock_t t = clock();
                            clock_t t2 = clock();
                            float dif = 0;
                            while(dif < tempo_linha){
                                t2 = clock();
                                dif = t2 - t;
                                // cout << dif << endl;
                            }
                        }
                    }else{
                        destinationImage.copyTo(destinationImage2);
                        for (i = 0; i < linhas_barcode_int; ++i)
                        {
                            //SE TIVER ALGUMA NOTA LIGADA E ELA ESTIVER NO VETOR DE NOTAS ANTERIOES, DESLIGÁ-LA
                            if(noteon == true){
                                for (int i = 0; i < (int)notas_anteriores.size(); ++i)
                                {
                                    message[0] = MIDI_NOTEOFF;
                                    message[1] = notas_anteriores[i];
                                    midiout->sendMessage( &message );
                                    noteon = false;
                                    tocada[notas_anteriores[i]] = false;
                                    // cout << "apagou nota " << notas_anteriores[i] << endl;
                                    notas_anteriores.erase(notas_anteriores.begin() + i);
                                }
                                // notas_anteriores.clear();
                            }

                            //SEPARAR GRID TRANSFORMADO EM POSIÇÕES PARA NOTAS
                            for (j = 0; j < 13; ++j)
                            {
                                //PRÓXIMA CÉLULA DO GRID NA ORDEM
                                celula = destinationImage(Rect((int)i*((float)destinationImage.rows/13), (int)(j*((float)destinationImage.cols/linhas_barcode_int)), (int)((float)destinationImage.rows/13), (int)((float)destinationImage.cols/linhas_barcode_int)));

                                //DESENHAR CÉLULAS LIDAS
                                rectangle(destinationImage2, Point(i*((float)destinationImage.rows/13) ,j*((float)destinationImage.cols/linhas_barcode_int)), Point((i+1)*((float)destinationImage.rows/13),(j+1)*((float)destinationImage.cols/linhas_barcode_int)), Scalar(0,255,0), 3);
                                imshow("Transformada",destinationImage2);


                                //FILTRAR O BRANCO DA CÉLULA
                                inRange(celula, Scalar(130, 130, 130), Scalar(255, 255, 255), mask1);

                                //POSIÇÕES PARA IDENTIFICAR SE HÁ ALGO DIFERENTE DE BRANCO NO CENTRO DA CÉLULA
                                for (int y = 11; y < mask1.rows-11; ++y)
                                {
                                    for (int x = 11; x < mask1.cols-11; ++x)
                                    {
                                        //VERIFICA SE O PIXEL ATUAL É PRETO
                                        Scalar colour = mask1.at<uchar>(Point(x, y));
                                        if(colour.val[0]==0){
                                            //SE O PIXEL FOR PRETO, LIGAR NOTA
                                            azul = true;

                                            //IDENTIFICAR SE A NOTA TOCANDO NO MOMENTO SERÁ DESLIGADA
                                            bool tem_nota = false;
                                            for (int a = 0; a < (int)notas_anteriores.size(); ++a)
                                            {
                                                if(notas_anteriores[a] == bateria_notas[j]){
                                                    tem_nota = true;
                                                }
                                            }

                                            //SE SIM, ELA PODE SER TOCADA NOVAMENTE
                                            if(tem_nota == true){
                                                // cout << "tem nota true" << endl;
                                                // cout << "tocou nota (seria apagada)" << oitava*16+13-j << endl;
                                                message[0] = MIDI_NOTEON_DRUM;
                                                message[1] = bateria_notas[j];
                                                midiout->sendMessage( &message );
                                                noteon = true; 
                                                tocada[oitava*20+12-j] = true;
                                                // tem_nota = false;
                                            }else{
                                                //SENÃO, CASO ELA AINDA NÃO TENHA SIDO TOCADA, TOCÁ-LA
                                                // cout << "tem nota false" << endl;
                                                if(tocada[oitava*20+12-j] == false){
                                                    // cout << "tocou nota" << endl; 
                                                    // cout << "tocou nota (mantem ligada)" << oitava*16+13-j << endl;   
                                                    message[0] = MIDI_NOTEON_DRUM;
                                                    message[1] = bateria_notas[j];
                                                    midiout->sendMessage( &message );
                                                    noteon = true; 
                                                    tocada[bateria_notas[j]] = true;
                                                }
                                                //SE A NOTA FOR LIGADA E NÃO ESTIVER NO ARRAY PARA DELISGAR, CONTINUA LIGADA
                                            }

                                            //SE OS PRÓXIMOS PIXELS DA DIREITA TAMBÉM FOREM PRETOS, NOTA NÃO SERÁ DESLIGADA A FORÇA
                                            for (int x1 = x; x1 < mask1.cols; ++x1)
                                            {
                                                Scalar colour2 = mask1.at<uchar>(Point(x1, 17));
                                                //AO IDENTIFICAR PRÓXIMO PIXEL BRANCO, DESLIGAR NOTA NA ETAPA DE DESLIGAR AS NOTAS
                                                if(colour2.val[0]!=0){
                                                    notas_anteriores.push_back(bateria_notas[j]);
                                                    // tocada[oitava*16+13-j] = false;
                                                }
                                            }
                                        }
                                        // imshow("mask1", mask1);
                                        // waitKey();
                                        // mask1.at<uchar>(Point(x, y)) = 0;

                                        //suavização da quebra de loop
                                        if(azul == true){
                                            x = mask1.cols;
                                            y = mask1.rows;
                                            azul = false;
                                        }
                                    }
                                }
                            }
                            // decodedObjects.clear();
                            switch(waitKey(1)){
                                case 27:
                                    goto final;
                                case 32:
                                    if(!stop){
                                        stop = true;
                                        cout << "STOP" << endl;
                                        shutNotes();
                                        decodedObjects.clear();
                                        decodificado = false;
                                        goto inicio;
                                    }
                                case 'q':
                                    goto final;
                            }
                            // usleep(tempo_linha);
                            clock_t t = clock();
                            clock_t t2 = clock();
                            float dif = 0;
                            while(dif < tempo_linha){
                                t2 = clock();
                                dif = t2 - t;
                                // cout << dif << endl;
                            }
                        }
                    }
                }

            }

        }

        // imshow("src", src);

        // namedWindow("copia com desenho");
        // moveWindow("copia com desenho", 1000,1000);
        imshow("VIDEO", image);

        //DETECÇÃO DE CONTROLES PRINCIPAL
        keyEx = waitKeyEx(50);
        if(keyEx == 27){
            break;
        }else if(keyEx == 113){
            break;
        }else if(keyEx == 32){
            if(stop){
                if(decodificado == true && ids.size() == 4){
                    stop = false;
                    cout << "PLAY" << endl;
                }else{
                    cout << "Posicione o layout corretamente!" << endl;
                }
            }
        }else if(keyEx == 65362){
            //TECLA PARA CIMA
            bpm = bpm+1;
            tempo = 60.0/(float)bpm;
            tempo_linha = (tempo*1000000);
            tempo_linha = tempo_linha/(float)barcode_int3;
            cout << "Batidas por minuto: " << bpm << endl;
        }else if(keyEx == 65364){
            //TACLA PARA BAIXO
            if(bpm > 0){
                bpm = bpm-1;
            }else{
                bpm = bpm;
            }
            tempo = 60.0/(float)bpm;
            tempo_linha = (tempo*1000000);
            tempo_linha = tempo_linha/(float)barcode_int3;
            cout << "Batidas por minuto: " << bpm << endl;
        }else if(keyEx == 65361){
            //TECLA PARA ESQUERDA
            if(oitava > 0){
                oitava = oitava-1;
            }else{
                oitava = oitava;
            }
            cout << "Oitava: " << oitava << endl;
        }else if(keyEx == 65363){
            //TECLA PARA DIREITA
            oitava = oitava+1;
            cout << "Oitava: " << oitava << endl;
        }else if(keyEx == 109){
            bateriaOn = !bateriaOn;
            if(bateriaOn == true){
                cout << "Modo de bateria ativado." << endl;
            }else{
                cout << "Modo de bateria desativado." << endl;
            }
        }
    }

    final:
    shutNotes();
    decodedObjects.clear();
    decodificado = false;

    return 0;
}