/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gr�fics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGPACSJOB_H
#define UDGPACSJOB_H

#include <QObject>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Thread>

#include "pacsdevice.h"

namespace udg {

using namespace ThreadWeaver;
/**
    Classe base de la qual herederan totes les operacions que es facin amb el PACS. Aquesta classe cont� els m�todes basics que s'han d'heredar.

    Aquesta classe hereda de ThreadWeaver::Job per aix� tenir autom�ticament la gesti� de les cues que implementa, i permetre que les operacions
    amb el PACS s'executin en un thread independent.

   @author Grup de Gr�fics de Girona  ( GGG ) <vismed@ima.udg.es>
*/
class PACSJob: public Job
{
Q_OBJECT
public:

    enum PACSJobType {SendDICOMFilesToPACSJobType, RetrieveDICOMFilesFromPACSJobType};

    ///Constructor de la classe
    PACSJob(PacsDevice pacsDevice);

    ///Retorna l'identificador del PACSJob aquest identificador �s �nic per tots els PACSJob
    int getPACSJobID();

    ///Retorna el PacsDevice amb el qual s'ha constru�t el PACSJob
    PacsDevice getPacsDevice();
    
    ///Indica quin tipus de PACSJob �s l'objecte
    virtual PACSJob::PACSJobType getPACSJobType() = 0;

signals:

    ///Signal que s'emet quan un PACSJob ha comen�at a executar-se
    void PACSJobStarted(PACSJob *);

    ///Signal que s'emet quan un PACSJob ha acabat d'executar-se
    void PACSJobFinished(PACSJob *);

private slots:

    ///Slot que s'activa quan el job actual de ThreadWeaver comen�a a executar-se
    void threadWeaverJobStarted();

    ///Slot que s'activa quan el job actual de ThreadWeaver ha finalitzat
    void threadWeaverJobDone();

private :

    static int m_jobIDCounter;
    int m_jobID;
    PacsDevice m_pacsDevice;

};

};

#endif
