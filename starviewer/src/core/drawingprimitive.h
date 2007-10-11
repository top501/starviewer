/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gràfics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGDRAWINGPRIMITIVE_H
#define UDGDRAWINGPRIMITIVE_H

#include <QObject>
#include <QColor>
#include <QString>
//Forward declarations
class vtkProp;


namespace udg {
//Forward declarations

/**
Classe base per a totes les primitives gràfiques que podem representar en un visualitzador

@author Grup de Gràfics de Girona  ( GGG )
*/

class DrawingPrimitive : public QObject{
    Q_OBJECT
public:

/// enumeració per saber quin estat té la primitiva
    enum { NORMAL, HIGHLIGHTED, SELECTED };
    
     /**Tipus de coordenades que podem tenir:
        DISPLAY -             x-y pixel values in window
        NORMALIZED DISPLAY -  x-y (0,1) normalized values
        VIEWPORT -            x-y pixel values in viewport
        NORMALIZED VIEWPORT - x-y (0,1) normalized value in viewport
        VIEW -                x-y-z (-1,1) values in camera coordinates. (z is depth)
        WORLD -               x-y-z global coordinate values
      */
    
    /**propietats bàsiques de les primitives:
        -color de la primitiva
        -color, per defecte, d'una primitiva
        -continuitat / discontinuitat de la primitiva
        -amplada del traç de la primitiva
        -opacitat de la primitiva
        -estat del background de les primitives tancades: actiu / inactiu.
        -sistema de coordenades que utilitza la primitiva
        -visibilitat de la primitiva
     */
    
    DrawingPrimitive( QObject *parent = 0 );
    ~DrawingPrimitive(){}
    
    ///permet assignar el color de la primitiva
    void setColor( QColor color )
    { m_primitiveColor = color; }
    
    ///ens retorna el color de la primitiva
    QColor getColor() const
    { return m_primitiveColor; }
    
    ///ens retorna l'alçada de la primitiva
    double getHeight() const
    { return m_height; }
    
    ///ens permet assignar l'alçada de la primitiva
    void setHeight( double height )
    {  m_height = height;  }
    
    ///ens retorna l'amplada de la primitiva
    double getWidth() const
    { return m_width; }
    
    ///ens permet assignar l'amplada de la primitiva
    void setWidth( double width )
    {  m_width = width;  }
    
    ///ens retorna l'opacitat de la primitiva
    double getOpacity() const
    { return m_opacity; }
    
    ///ens permet assignar l'opacitat de la primitiva
    void setOpacity( double opacity )
    { m_opacity = opacity; }
    
    ///assignem el sistema de coordenades desitjat
    void setCoordinatesSystem( QString coord )
    { m_coordinateSystem = coord; }

    ///retorna el sistema de coordenades com a QString
    QString getCoordinatesSystemAsString()
    {     return( m_coordinateSystem ); }
    
    ///Fem visible la primitiva
    void visibilityOn()
    { visibility( true ); }
    
    ///Fem invisible la primitiva
    void visibilityOff()
    { visibility( false ); }
    
    ///assignem la visibilitat amb un paràmetre
    void visibility( bool visibility )
    { m_visible = visibility; } 
    
     ///ens diu si la primitiva és visible o no
    bool isVisible()
    { return( m_visible ); }
    
    ///Fem highlight la primitiva
    void highlightOn()
    { m_state = HIGHLIGHTED; }
    
    ///Fem unhighlight la primitiva
    void highlightOff()
    { m_state = NORMAL; }
    
    ///ens diu si la primitiva està highlighted o no
    bool isHighlighted()
    { return( m_state == HIGHLIGHTED ); }
    
     ///Fem selecció de la primitiva
    void selectedOn()
    { m_state = SELECTED; }
    
    ///Fem deselecció de la primitiva
    void selectedOff()
    { m_state = NORMAL; }
    
    ///ens diu si la primitiva està seleccionada o no
    bool isSelected()
    { return( m_state == SELECTED ); }
    
     ///ens diu si la primitiva està en estat normal
    bool isNormal()
    { return( m_state == NORMAL ); }
    
    ///ens diu el tipus de primitiva que és. (reimplementat en les sub-classes)
    virtual QString getPrimitiveType() = 0;
    
    ///ens retorna l'estat
    int getState()
    { return( m_state ); }
protected:
   
    ///color de la primitiva
    QColor m_primitiveColor;
    
    ///determina l'alçada de la primitiva
    double m_height;
    
    ///determina l'amplada de la primitiva
    double m_width;
    
    ///determina l'opacitat de la primitiva
    double m_opacity;
    
    ///sistema de coordenades que utilitza la primitiva
    QString m_coordinateSystem;

    ///visibilitat de la primitiva
    bool m_visible;
    
    ///estat de la primitiva
    int m_state;
};

};  
#endif

