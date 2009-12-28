/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gràfics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGQEXECUTEOPERATIONTHREAD_H
#define UDGQEXECUTEOPERATIONTHREAD_H

#include <QThread>

#include "operation.h"

class QSemaphore;

namespace udg {

/** Aquest classe, s'encarrega d'anar executant objectes Operation. (operacions que s'han de dur a terme). Aquesta classe crea un thread quan hi ha alguna operacio i les executa. A més també utilitza una cua, on es van guardant totes les operation pendents d'executar
	@author Grup de Gràfics de Girona  ( GGG ) <vismed@ima.udg.es>
*/

class Status;
class LocalDatabaseManagerThreaded;
class PatientFiller;
class QThreadRunWithExec;
class StarviewerProcessImageRetrieved;
class LocalDatabaseManager;
class Image;

class QExecuteOperationThread : public QThread
{
Q_OBJECT

public:

    ///Es defineix els tipus d'error que podem tenir, el DatabaseError indica quan és error de Sqlite
    enum RetrieveError {DatabaseError, CanNotConnectPacsToMove, NoEnoughSpace, ErrorFreeingSpace, PatientInconsistent, MoveDestinationAETileUnknownStatus, 
                         IncomingConnectionsPortPacsInUse, MoveFailureOrRefusedStatus, MoveUnknowStatus};

    //MoveWarningStatus - Per a Movescu indica que alguna imatge no s'ha pogut descarrega
    enum RetrieveWarning {MoveWarningStatus};

    //StoreFailureStatus - L'enviament de totes les imatges ha fallat
    enum StoreError {CanNotConnectPacsToStore, StoreFailureStatus, StoreUnknowStatus};

    /* StoreSomeImagesFailureStatus - L'enviament d'algunes imatges ha fallat    
       StoreWarningStatus - Per a StoreSCU indica que totes les imatges s'han enviat però per totes o alguna imatge hem rebut un warning */
    enum StoreWarning {StoreWarningStatus, StoreSomeImagesFailureStatus};

    /** Constructor de la classe
      */
    QExecuteOperationThread( QObject *parent = 0 );

    /** Destructor de la classe
     */
    ~QExecuteOperationThread();

    /** afegeix una operacio a la cua, si la cua esta buida l'afegeix i crea el thread que l'executarà
      *@param Operation a executar. Si l'operació de moure imatges cap un PACS, a la màscara de l'estudi de l'operació només caldrà especificar el studyUID
      */
    void queueOperation( Operation operation );

    /** Codi que s'executa en un nou thread, evalua l'operacio i decideix que fer
      */
    void run();

signals:

    /** signal que s'emet cap a QueryScreen quant s'ha descarregat la primera serie d'un estudi per a que pugui ser visualitzat
     * @param studyUID UID de l'estudi a visualitzar
     * @param seriesUID de la imatge a visualitzar
     * @param imageUID de l'imatge a visualitzar
     */
    void viewStudy( QString studyUID , QString seriesUID , QString imageUID );

    /**
     * Aquest signal s'emetrà quan volguem que un estudi descarregat s'afegeixi a les extensions però en background
     * sense necessitat de que s'apliqui cap canvi visible en la presentació dels estudis, simplement fusionarà la informació
     * del pacient i prou, necessari per funcionalitats com la càrrega d'estudis previs, per exemple.
     */
    void loadStudy( QString studyUID , QString seriesUID , QString imageUID );

    /** signal que s'emet cap a QRetrieveScreen per indicar que l'estudi s'està descarregant
     * @param studyUID UID de l'estudi que s'està descarregant
     */
    void setOperating( QString studyUID );

    /** signal que s'emet cap a QRetrieveScreen per indicar que la operacio a finalitzatt
     * @param studyUID UID de l'estudi descarregat
     */
    void setOperationFinished( QString studyUID );

    ///signal que s'emet cap a QueryScreen per indicar que l'estudi s'ha descarregat i s'ha processat
    void retrieveFinished( QString studyUID );

    ///Indica que ha comença la descarrega dimatges DICOM
    void retrieveStarted( QString studyUID );

    ///Indiquem que la operació serà cancel·lada
    void setCancelledOperation(QString studyInstanceUID);

    /** signal que s'emet cap a QRetrieveScreen per indicar que s'ha descarregat una nova imatge de l'estudi
     * @param studyUID UID de l'estudi que s'està descarregant
     * @param número d'imatge descarrega
     */
    void imageCommit( QString studyUID , int );

    /// Signal que s'emet quan canvia el nombre d'imatges que s'han descarregats del study que s'està processant actualment.
    /// L'study actual es pot saber a partir del signal setOperating
    void currentProcessingStudyImagesRetrievedChanged(int imagesRetrieved);

    /** signal que s'emet cap a QRetrieveScreen per indicar que s'ha descarregat una nova sèroe de l'estudi
     * @param studyUID UID de l'estudi que s'esta descarregat
     */
    void seriesCommit( QString );

    /** signal que s'emet quan s'enqua una nova operació
     * @param newOperation operació encuada
     */
    void newOperation( Operation *newOperation );

    ///Signal que s'emet quan es produeix un error a l'operació de descàrrega
    void errorInRetrieve(QString studyUID, QString pacsID, QExecuteOperationThread::RetrieveError);

    void warningInRetrieve(QString studyUID, QString pacsID, QExecuteOperationThread::RetrieveWarning);

    ///Signal que s'emet quan s'han descarregat tots els fitxers d'un estudi
    void filesRetrieved();

    ///Signal que s'emet per indicar que un estudi serà esborrat de la base de dades perquè no es disposa de prou espai
    void studyWillBeDeleted(QString studyInstanceUID);

    //S'ha produït un error al intentar enviar un estudi al PACS
    void errorInStore(QString studyUID, QString pacsID, QExecuteOperationThread::StoreError);

    //S'ha produït un warning al enviar un estudi al PACS
    void warningInStore(QString studyUID, QString pacsID, QExecuteOperationThread::StoreWarning);

private slots:

    ///Slot que fa un signal conforme s'ha descarregat una sèrie de l'estudi passat per paràmetre
    void seriesRetrieved(QString studyInstanceUID);

	///emet el signa del que un estudi serà esborrar studyWillBeDeleted
	void studyWillBeDeletedSlot(QString studyInstanceUID);

private:

    /** Descarrega un estudi, segons els paràmetres a operation, si l'estudi s'ha de visualitzar
      * captura els signals de l'objecte starviewersettings que es emes cada vegada que finalitza la descarrega d'una  serie
      *     @param operation a executar
      */
    void retrieveStudy(Operation operation);

    /** Mou un estudi descarregat al pacs especificat
     * @param operation paràmetres de l'operació que s'ha de dur a terme. Si � un Move a la màscara només cal especificar el studyUID
     * @return estat del mètode
     */
    void moveStudy( Operation operation );

private:
    bool m_stoppedThread;//indica si el thread esta parat
    QList <Operation> m_queueOperationList;
    QSemaphore *m_qsemaphoreQueueOperationList; //controla l'accés a la QueueOperationList

    ///Retorna la pròxima operacio de més prioritat pedent d'executar, si hi ha dos que tenen la mateixa prioritat retorna la que porta més temps a la cua
    Operation takeMaximumPriorityOperation();

    ///Cancel·la les operacions pendents del tipus passat per paràmetre
    void cancelAllPendingOperations(Operation::OperationAction cancelPendingOperations);

    //Crea les connexions de signals i slots necessaries per a descarregar un estudi
    void createRetrieveStudyConnections(LocalDatabaseManager *localDatabaseManager, LocalDatabaseManagerThreaded *localDatabaseManagerThreaded, PatientFiller *patientFiller, QThreadRunWithExec *fillersThread, StarviewerProcessImageRetrieved *starviewerProcessImageRetrieved);

    ///Si es produeix un error emet un signal amb l'error i esborra el directori de l'estudi per si s'hagués pogut descarregar alguna imatge
    void errorRetrieving(QString studyInstanceUID, QString pacsID, QExecuteOperationThread::RetrieveError lastError);

    ///Indica si l'estudi existeix a la base de dades local
    bool existStudyInLocalDatabase(QString studyInstanceUID);

    ///Retorna la llista d'imatges a guardar a un PACS a partir de la màscara
    QList<Image*> getImagesToStoreList(DicomMask dicomMask);
};

}

#endif
