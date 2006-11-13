/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gr�fics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGQ3DMPRVIEWER_H
#define UDGQ3DMPRVIEWER_H

#include "qviewer.h"

class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkImagePlaneWidget;
class vtkActor;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkCommand;
class vtkLookupTable;

namespace udg {

/**
    Visor de Reconstrucci� multiplanar 3D

@author Grup de Gr�fics de Girona  ( GGG )
*/

// FWD declarations
class Q3DOrientationMarker;
class Q3DMPRViewerToolManager;

class Q3DMPRViewer : public QViewer{
Q_OBJECT
public:
    Q3DMPRViewer( QWidget *parent = 0 );

    ~Q3DMPRViewer();

    /// Li indiquem el volum a visualitzar
    virtual void setInput( Volume *inputImage );

    virtual vtkRenderer *getRenderer();

    /// Retorna el volum transformat segons el reslice de cada vista
    Volume *getAxialResliceOutput();
    Volume *getSagitalResliceOutput();
    Volume *getCoronalResliceOutput();

    /// M�todes per obtenir les coordenades que defineixen els plans
    double *getAxialPlaneOrigin();
    double *getAxialPlaneNormal();
    void getAxialPlaneOrigin( double origin[3] );
    void getAxialPlaneNormal( double normal[3] );

    double *getSagitalPlaneOrigin();
    double *getSagitalPlaneNormal();
    void getSagitalPlaneOrigin( double origin[3] );
    void getSagitalPlaneNormal( double normal[3] );

    double *getCoronalPlaneOrigin();
    double *getCoronalPlaneNormal();
    void getCoronalPlaneOrigin( double origin[3] );
    void getCoronalPlaneNormal( double normal[3] );

    /// Assigna la LUT en format vtk
    void setVtkLUT( vtkLookupTable *lut );

    /// Retorna la LUT en format vtk
    vtkLookupTable *getVtkLUT();

    /// Retorna el window level
    void getWindowLevel( double wl[2] );

signals:
    /// senyal que indica que algun dels plans han canviat
    void planesHasChanged( void );

public slots:
    /// Inicialitza la vista de la c�mara per veure el model des d'una orientaci� per defecte determinada ( acial , sagital o coronal )
    void resetViewToSagital();
    void resetViewToCoronal();
    void resetViewToAxial();

    /// Habilitar/Deshabilitar la visibilitat d'un dels plans
    void setSagitalVisibility( bool enable );
    void setCoronalVisibility( bool enable );
    void setAxialVisibility( bool enable );

    /// Reinicia de nou els plans
    void resetPlanes();

    /// Ajusta el window/level
    void setWindowLevel( double window , double level );

    /// M�todes per donar diversos window level per defecte
    void resetWindowLevelToDefault();
    void resetWindowLevelToBone();
    void resetWindowLevelToEmphysema();
    void resetWindowLevelToSoftTissuesNonContrast();
    void resetWindowLevelToLiverNonContrast();
    void resetWindowLevelToSoftTissuesContrastMedium();
    void resetWindowLevelToLiverContrastMedium();
    void resetWindowLevelToNeckContrastMedium();
    void resetWindowLevelToAngiography();
    void resetWindowLevelToOsteoporosis();
    void resetWindowLevelToPetrousBone();
    void resetWindowLevelToLung();

    /// m�todes per controlar la visibilitat de l'outline
    void enableOutline( bool enable );
    void outlineOn();
    void outlineOff();

    /// m�todes per controlar la visibilitat de l'orientation marker widget
    void enableOrientationMarker( bool enable );
    void orientationMarkerOn();
    void orientationMarkerOff();

    /// chapussa per agafar els events dels image plane widgets i enviar una senya conforme han canviat \TODO mirar si es pot millorar un m�tode en comptes de fer aix�
    void planeInteraction();

    void setEnableTools( bool enable );
    void enableTools();
    void disableTools();

    virtual void render();
    void reset();

private:
    /// Els respectius volums sobre cada pla de reslice
    Volume *m_axialResliced, *m_sagitalResliced , *m_coronalResliced;

    /// Crea l'actor que mostra una refer�ncia de l'orientaci� dels eixos
    void setCameraOrientation( int orientation );

    /// inicialitza els valors de window level
    void initializeWindowLevel();

    /// Inicialitza els plans
    void initializePlanes();

    /// Actualitza les dades sobre les que tracten els plans
    void updatePlanesData();

    /// Afegeix l'outline de la boundingbox del model
    void createOutline();

    /// Crea tots els actors que intervenen en l'escena
    void createActors();

    /// Afegeix els actors a l'escena
    void addActors();

    enum {SAGITAL, CORONAL, AXIAL};

    /// El renderer
    vtkRenderer *m_renderer;

    /// Els plans
    vtkImagePlaneWidget *m_axialImagePlaneWidget;
    vtkImagePlaneWidget *m_sagitalImagePlaneWidget;
    vtkImagePlaneWidget *m_coronalImagePlaneWidget;

    /// La bounding box del volum
    vtkActor *m_outlineActor;

    /// Widget per veure la orientaci� en 3D
    Q3DOrientationMarker *m_orientationMarker;

    /// connexions d'events vtk amb slots / signals qt
    vtkEventQtSlotConnect *m_vtkQtConnections;

    /// Valors dels window level per defecte. Pot venir donat pel DICOM o assignat per nosaltres a un valor est�ndar de constrast
    double m_defaultWindow , m_defaultLevel;

    /// control de visibilitat dels plans
    bool m_axialPlaneVisible, m_sagitalPlaneVisible , m_coronalPlaneVisible;

    /// control de visibilitat de l'outline i l'orientation marker widget \TODO �s possible que aquests membres acabin sent superflus i innecessaris
    bool m_isOutlineEnabled;

    /// Tool Manager del visor
    Q3DMPRViewerToolManager *m_toolManager;
};

};  //  end  namespace udg

#endif
