// TODO
// - Calcular el gradient i guardar-lo -> frambebuffer 3D, llesca a llesca
// - Per combinar shaders:
//   · shader bàsic de ray casting que cridi un mètode shade(...) que retorni el color del voxel
//   · aquest shade(...) s'implementarà en un altre fitxer o passant-li directament l'string depenent de les necessitats;
//     hauria de ser un mètode que crides "shades" més específics i els combinés; per exemple:
//     * vec4 shade(...) { return shadeAmbient(...); }
//     * vec4 shade(...) { return shadeObscurance(shadeDiffuse(...),...); }
//       ...
//   · cadascun d'aquests "shades" més específics estaria implementat al seu propi fitxer
//   · al final, al Shader se li haurien d'afegir tots els fragment shaders que calguessin per tenir totes les funcions necessàries definides;
//     exemple: bàsic + shade combinatori + shades específics utilitzats pel combinatori
/// \bug no sempre es carreguen bé els shaders (falla aleatòriament); provar de recompilar a veure si encara passa



#include "qgputestingviewer.h"

#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTextStream>
#include <QWheelEvent>

#include <vtkImageData.h>

#include "camera.h"
#include "gpuprogram.h"
#include "volume.h"


namespace udg {


const float QGpuTestingViewer::KeyboardCameraIncrement = 10.0f;
const float QGpuTestingViewer::MaximumCameraDistanceFactor = 1000.0f;


QGpuTestingViewer::QGpuTestingViewer( QWidget *parent )
 : QGLWidget( parent ), m_extensions( false ), m_volume( 0 ), m_camera( 0 ), m_fieldOfView( 30.0 ), m_vertexBufferObject( 0 ),
   m_volumeTexture( 0 ), m_framebufferObject( 0 ), m_framebufferTexture( 0 ), m_gpuProgram( 0 ), m_reloadShaders( false ),
   m_backgroundColor( Qt::transparent ), m_rayStep( 1.0 ), m_lighting( Ambient ), m_specularPower( 64.0f ), m_transferFunctionTexture( 0 )
{
    setFocusPolicy( Qt::WheelFocus );

    m_transferFunction.set(0.0, Qt::black, 0.0);
    m_transferFunction.set(1.0, Qt::white, 1.0);
}


QGpuTestingViewer::~QGpuTestingViewer()
{
    if ( m_extensions )
    {
        delete m_camera;
        glDeleteBuffersARB( 1, &m_vertexBufferObject );
        glDeleteTextures( 1, &m_volumeTexture );
        glDeleteFramebuffersEXT( 1, &m_framebufferObject );
        glDeleteTextures( 1, &m_framebufferTexture );
        delete m_gpuProgram;
        glDeleteTextures( 1, &m_transferFunctionTexture );
    }
}


// es crida abans de l'initializeGL
void QGpuTestingViewer::setVolume( Volume *volume )
{
    m_volume = volume;

    int *dimensions = volume->getDimensions();
    double *spacing = volume->getSpacing();
    m_dimX = dimensions[0] * spacing[0]; m_dimY = dimensions[1] * spacing[1]; m_dimZ = dimensions[2] * spacing[2];
    m_biggestDimension = qMax( qMax( m_dimX, m_dimY ), m_dimZ );
    m_diagonalLength = Vector3( m_dimX, m_dimY, m_dimZ ).length();

    m_keyboardZoomIncrement = 0.1f * m_diagonalLength;
    m_wheelZoomScale = m_diagonalLength / 720.f;
}


const QColor& QGpuTestingViewer::backgroundColor() const
{
    return m_backgroundColor;
}


void QGpuTestingViewer::setBackgroundColor( const QColor &backgroundColor )
{
    m_backgroundColor = backgroundColor;
    m_backgroundColor.setAlpha( 0 );
    qglClearColor( m_backgroundColor );
}


void QGpuTestingViewer::setRayStep( float rayStep )
{
    m_rayStep = rayStep;
}


void QGpuTestingViewer::setLighting( bool diffuse, bool specular, float specularPower )
{
    Lighting lighting;

    if ( !diffuse ) lighting = Ambient;
    else if ( !specular ) lighting = Diffuse;
    else lighting = DiffuseSpecular;

    if ( m_lighting != lighting )
    {
        m_lighting = lighting;
        m_reloadShaders = true;
    }

    m_specularPower = specularPower;
}


void QGpuTestingViewer::setTransferFunction( const TransferFunction &transferFunction )
{
    m_transferFunction = transferFunction;
    updateTransferFunctionTexture();
}


void QGpuTestingViewer::getCamera( Vector3 &position, Vector3 &focus, Vector3 &up ) const
{
    position = m_camera->getPosition();
    focus = position + m_camera->getViewDirection() * m_camera->getOrbitOffsetDistance();
    up = m_camera->getYAxis();
}



void QGpuTestingViewer::setCamera( const Vector3 &position, const Vector3 &focus, const Vector3 &up )
{
    m_camera->lookAt( position, focus, up );
    m_camera->setOrbitOffsetDistance( ( position - focus ).length() );
    updateGL();
}


void QGpuTestingViewer::setFieldOfView( int fieldOfView )
{
    m_fieldOfView = fieldOfView;
    updateGL();
}


void QGpuTestingViewer::keyPressEvent( QKeyEvent *event )
{
    switch ( event->key() )
    {
        case Qt::Key_Left: m_camera->rotateSmoothly( KeyboardCameraIncrement, 0.0f, 0.0f ); break;
        case Qt::Key_Right: m_camera->rotateSmoothly( -KeyboardCameraIncrement, 0.0f, 0.0f ); break;
        case Qt::Key_Up: m_camera->rotateSmoothly( 0.0f, KeyboardCameraIncrement, 0.0f ); break;
        case Qt::Key_Down: m_camera->rotateSmoothly( 0.0f, -KeyboardCameraIncrement, 0.0f ); break;
        case Qt::Key_Plus: m_camera->zoom( -m_keyboardZoomIncrement, m_diagonalLength / 2.0f + 1.0f, m_biggestDimension * MaximumCameraDistanceFactor ); break;
        case Qt::Key_Minus: m_camera->zoom( m_keyboardZoomIncrement, m_diagonalLength / 2.0f + 1.0f, m_biggestDimension * MaximumCameraDistanceFactor ); break;
        case Qt::Key_R: resetCamera(); break;
        case Qt::Key_F10: loadShaders(); break;
        default: QWidget::keyPressEvent( event ); return;
    }

    updateGL();
}


void QGpuTestingViewer::mousePressEvent( QMouseEvent *event )
{
    m_lastX = event->x(); m_lastY = event->y();
}


void QGpuTestingViewer::mouseMoveEvent( QMouseEvent *event )
{
    int dx = event->x() - m_lastX;
    int dy = event->y() - m_lastY;

    m_camera->rotateSmoothly( -dx, -dy, 0.0f );

    m_lastX = event->x(); m_lastY = event->y();

    updateGL();
}


void QGpuTestingViewer::wheelEvent( QWheelEvent *event )
{
    m_camera->zoom( -m_wheelZoomScale * event->delta(), m_diagonalLength / 2.0f + 1.0f, m_biggestDimension * MaximumCameraDistanceFactor );
    updateGL();
}


void QGpuTestingViewer::initializeGL()
{
    GLenum glew = glewInit();

    if ( glew == GLEW_OK )
    {
        // Comprovació de les extensions

        QString nonSupportedExtensions;

        if ( !GLEW_ARB_multitexture ) nonSupportedExtensions += "GL_ARB_multitexture\n";
        if ( !GLEW_ARB_texture_non_power_of_two ) nonSupportedExtensions += "GL_ARB_texture_non_power_of_two\n";
        if ( !GLEW_ARB_vertex_shader ) nonSupportedExtensions += "GL_ARB_vertex_shader\n";
        if ( !GLEW_ARB_fragment_shader ) nonSupportedExtensions += "GL_ARB_fragment_shader\n";
        if ( !GLEW_ARB_shader_objects ) nonSupportedExtensions += "GL_ARB_shader_objects\n";
        if ( !GLEW_ARB_shading_language_100 ) nonSupportedExtensions += "GL_ARB_shading_language_100\n";
        if ( !GLEW_ARB_vertex_buffer_object ) nonSupportedExtensions += "GL_ARB_vertex_buffer_object\n";
        if ( !GLEW_EXT_framebuffer_object ) nonSupportedExtensions += "GL_EXT_framebuffer_object\n";

        if ( nonSupportedExtensions.isNull() ) m_extensions = true;
        else
        {
            DEBUG_LOG( "Extensions no suportades:\n" + nonSupportedExtensions.trimmed() );
            QMessageBox::warning( this, tr("Non-supported extensions"),
                                  tr("The GPU testing extension won't work as expected because your system doesn't support the following OpenGL extensions:") + "\n" + nonSupportedExtensions.trimmed() );
        }
    }
    else DEBUG_LOG( QString( "Ha fallat el glewInit() amb l'error: %1" ).arg( reinterpret_cast<const char*>( glewGetErrorString( glew ) ) ) );

    // Color i profunditat inicials
    qglClearColor( m_backgroundColor );
    glClearDepth( 1.0 );

    if ( m_extensions )
    {
        createCamera();
        createVertexBufferObject();
        createVolumeTexture();
        createFramebufferObject();
        loadShaders();
        createTransferFunctionTexture();
        updateTransferFunctionTexture();
    }

    glEnable( GL_CULL_FACE );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
}


void QGpuTestingViewer::resizeGL( int width, int height )
{
    glViewport( 0, 0, width, height );

    if ( !m_extensions ) return;

    recreateFramebufferTexture();
    adjustProjection();
}


void QGpuTestingViewer::paintGL()
{
    if ( !m_extensions )
    {
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        return;
    }

    adjustProjection();

    // Primer pintem les cares del darrere al framebuffer
    firstPass();

    // Després pintem les cares del davant amb els shaders
    secondPass();
}


void QGpuTestingViewer::checkGLError( bool alert )
{
    GLenum error = glGetError();

    if ( error != GL_NO_ERROR )
    {
        QString errorString( reinterpret_cast<const char*>( gluErrorString( error ) ) );
        DEBUG_LOG( errorString );
        if ( alert ) QMessageBox::warning( this, tr("OpenGL error"), errorString );
    }
}


void QGpuTestingViewer::createCamera()
{
    m_camera = new Camera();
    resetCamera();
}


void QGpuTestingViewer::resetCamera()
{
    const Vector3 EYE( 0.0, 0.0, 2.0f * m_biggestDimension );

    m_camera->setBehavior( Camera::CAMERA_BEHAVIOR_ORBIT );
    m_camera->setPreferTargetYAxisOrbiting( false );
    m_camera->setOrbitOffsetDistance( EYE.length() );   // ha de ser la distància inicial entre la càmera i l'objectiu

    m_camera->lookAt( EYE, Vector3(), Vector3( 0.0, 1.0, 0.0 ) );   // posició inicial
}


void QGpuTestingViewer::createVertexBufferObject()
{
    GLfloat x = m_dimX / 2.0f, y = m_dimY / 2.0f, z = m_dimZ / 2.0f;

    VertexBufferObjectData data[24] =   // 4 vèrtexs/cara * 6 cares
    {   //  nx     ny     nz   |   r      g      b    |  x   y   z
        { -1.0f,  0.0f,  0.0f,    0.0f,  0.0f,  0.0f,   -x, -y, -z },
        { -1.0f,  0.0f,  0.0f,    0.0f,  0.0f,  1.0f,   -x, -y, +z },
        { -1.0f,  0.0f,  0.0f,    0.0f,  1.0f,  1.0f,   -x, +y, +z },
        { -1.0f,  0.0f,  0.0f,    0.0f,  1.0f,  0.0f,   -x, +y, -z },

        {  0.0f,  0.0f, -1.0f,    0.0f,  0.0f,  0.0f,   -x, -y, -z },
        {  0.0f,  0.0f, -1.0f,    0.0f,  1.0f,  0.0f,   -x, +y, -z },
        {  0.0f,  0.0f, -1.0f,    1.0f,  1.0f,  0.0f,   +x, +y, -z },
        {  0.0f,  0.0f, -1.0f,    1.0f,  0.0f,  0.0f,   +x, -y, -z },

        {  0.0f, -1.0f,  0.0f,    0.0f,  0.0f,  0.0f,   -x, -y, -z },
        {  0.0f, -1.0f,  0.0f,    1.0f,  0.0f,  0.0f,   +x, -y, -z },
        {  0.0f, -1.0f,  0.0f,    1.0f,  0.0f,  1.0f,   +x, -y, +z },
        {  0.0f, -1.0f,  0.0f,    0.0f,  0.0f,  1.0f,   -x, -y, +z },

        {  0.0f,  0.0f,  1.0f,    0.0f,  0.0f,  1.0f,   -x, -y, +z },
        {  0.0f,  0.0f,  1.0f,    1.0f,  0.0f,  1.0f,   +x, -y, +z },
        {  0.0f,  0.0f,  1.0f,    1.0f,  1.0f,  1.0f,   +x, +y, +z },
        {  0.0f,  0.0f,  1.0f,    0.0f,  1.0f,  1.0f,   -x, +y, +z },

        {  0.0f,  1.0f,  0.0f,    0.0f,  1.0f,  0.0f,   -x, +y, -z },
        {  0.0f,  1.0f,  0.0f,    0.0f,  1.0f,  1.0f,   -x, +y, +z },
        {  0.0f,  1.0f,  0.0f,    1.0f,  1.0f,  1.0f,   +x, +y, +z },
        {  0.0f,  1.0f,  0.0f,    1.0f,  1.0f,  0.0f,   +x, +y, -z },

        {  1.0f,  0.0f,  0.0f,    1.0f,  0.0f,  0.0f,   +x, -y, -z },
        {  1.0f,  0.0f,  0.0f,    1.0f,  1.0f,  0.0f,   +x, +y, -z },
        {  1.0f,  0.0f,  0.0f,    1.0f,  1.0f,  1.0f,   +x, +y, +z },
        {  1.0f,  0.0f,  0.0f,    1.0f,  0.0f,  1.0f,   +x, -y, +z }
    };

    glGenBuffersARB( 1, &m_vertexBufferObject );
    glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_vertexBufferObject );
    glBufferDataARB( GL_ARRAY_BUFFER_ARB, 24 * sizeof( VertexBufferObjectData ), data, GL_STATIC_DRAW_ARB );

    checkGLError();
}


void QGpuTestingViewer::createVolumeTexture()
{
    // Primer passem el volum a reals i escalat entre 0 i 1

    vtkImageData *imageData = m_volume->getVtkData();
    double *range = imageData->GetScalarRange();
    float min = range[0], max = range[1];
    float shift = -min, scale = 1.0 / ( max - min );
    int size = imageData->GetNumberOfPoints();
    short *data = reinterpret_cast<short*>( imageData->GetScalarPointer() );
    float *floatData = new float[size];

    for ( int i = 0; i < size; i++ ) floatData[i] = ( data[i] + shift ) * scale;

    // Després creem la textura 3D

    int *dimensions = m_volume->getDimensions();

    glGenTextures( 1, &m_volumeTexture );
    glBindTexture( GL_TEXTURE_3D, m_volumeTexture );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER );
    glTexImage3D( GL_TEXTURE_3D, 0, GL_INTENSITY16F_ARB, dimensions[0], dimensions[1], dimensions[2], 0, GL_RED, GL_FLOAT, floatData );
    checkGLError( true );

    delete[] floatData;
}


void QGpuTestingViewer::createFramebufferObject()
{
    glGenFramebuffersEXT( 1, &m_framebufferObject );
    checkGLError();

    createFramebufferTexture();
}


void QGpuTestingViewer::createFramebufferTexture()
{
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_framebufferObject );

    glGenTextures( 1, &m_framebufferTexture );
    glBindTexture( GL_TEXTURE_2D, m_framebufferTexture );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width(), height(), 0, GL_RGBA, GL_FLOAT, 0 );
    glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_framebufferTexture, 0 );
    checkGLError();

    GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
    if ( status == GL_FRAMEBUFFER_COMPLETE_EXT ) DEBUG_LOG( "framebuffer ok :D" );
    else DEBUG_LOG( "framebuffer ko :( " + QString::number( status, 16 ) );

    checkGLError();
}


void QGpuTestingViewer::recreateFramebufferTexture()
{
    glDeleteTextures( 1, &m_framebufferTexture );
    createFramebufferTexture();
}


void QGpuTestingViewer::loadShaders()
{
    if ( !m_extensions ) return;

    DEBUG_LOG( "loadShaders()" );

    delete m_gpuProgram;
    m_gpuProgram = new GpuProgram();
    m_gpuProgram->addVertexShaderFromFile( ":/extensions/GpuTestingExtension/shaders/shader.vert" );
    m_gpuProgram->addFragmentShaderFromFile( ":/extensions/GpuTestingExtension/shaders/shader.frag" );
    //m_gpuProgram->addFragmentShaderFromFile( "/scratch/starviewer/src/extensions/playground/gputesting/shaders/shader.frag" );

    if ( m_lighting == Ambient )
    {
        m_gpuProgram->addFragmentShaderFromFile( ":/extensions/GpuTestingExtension/shaders/ambientshader.frag" );
        //m_gpuProgram->addFragmentShaderFromFile( "/scratch/starviewer/src/extensions/playground/gputesting/shaders/ambientshader.frag" );
        m_gpuProgram->addFragmentShader( "vec4 ambientShade(vec3);"
                                         "vec4 shade(vec3 coord,vec3 u,vec3 d){return ambientShade(coord);}" );
    }
    else
    {
        m_gpuProgram->addFragmentShaderFromFile( ":/extensions/GpuTestingExtension/shaders/diffuseshader.frag" );
        //m_gpuProgram->addFragmentShaderFromFile( "/scratch/starviewer/src/extensions/playground/gputesting/shaders/diffuseshader.frag" );
        m_gpuProgram->addFragmentShader( "vec4 diffuseShade(vec3,vec3,vec3);"
                                         "vec4 shade(vec3 coord,vec3 unit,vec3 direction){return diffuseShade(coord,unit,direction);}" );
    }

    m_gpuProgram->link();

    if ( !m_gpuProgram->isValid() )
    {
        DEBUG_LOG( "Hi ha hagut algun problema en crear el shader" );
        return;
    }

    if ( !m_gpuProgram->initUniform( "uFramebufferTexture" ) ) DEBUG_LOG( "Error en obtenir el framebuffer texture uniform" );
    if ( !m_gpuProgram->initUniform( "uDimensions" ) ) DEBUG_LOG( "Error en obtenir el dimensions uniform" );
    if ( !m_gpuProgram->initUniform( "uRayStep" ) ) DEBUG_LOG( "Error en obtenir el ray step uniform" );
    if ( !m_gpuProgram->initUniform( "uBackgroundColor" ) ) DEBUG_LOG( "Error en obtenir el background color uniform" );
    if ( !m_gpuProgram->initUniform( "uVolumeTexture" ) ) DEBUG_LOG( "Error en obtenir el volume texture uniform" );
    if ( !m_gpuProgram->initUniform( "uTransferFunctionTexture" ) ) DEBUG_LOG( "Error en obtenir el transfer function texture uniform" );

    checkGLError();
}


void QGpuTestingViewer::createTransferFunctionTexture()
{
    glGenTextures( 1, &m_transferFunctionTexture );
    glBindTexture( GL_TEXTURE_1D, m_transferFunctionTexture );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
    checkGLError();
}


void QGpuTestingViewer::updateTransferFunctionTexture()
{
    struct { GLubyte r, g, b, a; } pixels[TransferFunctionTextureSize];
    double scale = 1.0 / ( TransferFunctionTextureSize - 1 );

    for ( int i = 0; i < TransferFunctionTextureSize; i++ )
    {
        QColor color = m_transferFunction.get( i * scale );
        pixels[i].r = color.red();
        pixels[i].g = color.green();
        pixels[i].b = color.blue();
        pixels[i].a = color.alpha();
    }

    glBindTexture( GL_TEXTURE_1D, m_transferFunctionTexture );
    glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA, TransferFunctionTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    checkGLError();
}


void QGpuTestingViewer::adjustProjection()
{
    int width = this->width(), height = this->height();

    if ( height == 0 ) height = 1;

    float cameraDistance = m_camera->getPosition().length();
    float zNear = cameraDistance - m_diagonalLength / 2.0f;
    float zFar = cameraDistance + m_diagonalLength / 2.0f;

    m_camera->perspective( m_fieldOfView, static_cast<float>( width ) / static_cast<float>( height ), zNear, zFar );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glMultMatrixf( &( m_camera->getProjectionMatrix()[0][0] ) );
}


void QGpuTestingViewer::firstPass()
{
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_framebufferObject );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMultMatrixf( &( m_camera->getViewMatrix()[0][0] ) );

    glCullFace( GL_FRONT );

    drawVertexBufferObject();

    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
}


void QGpuTestingViewer::secondPass()
{
    if ( m_reloadShaders )
    {
        loadShaders();
        m_reloadShaders = false;
    }

    glUseProgramObjectARB( m_gpuProgram->programObject() );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMultMatrixf( &( m_camera->getViewMatrix()[0][0] ) );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_framebufferTexture );
    glUniform1iARB( m_gpuProgram->uniform( "uFramebufferTexture" ), 0 );

    glUniform3fARB( m_gpuProgram->uniform( "uDimensions" ), m_dimX, m_dimY, m_dimZ );

    glUniform1fARB( m_gpuProgram->uniform( "uRayStep" ), m_rayStep );

    glUniform3fARB( m_gpuProgram->uniform( "uBackgroundColor" ), m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF() );

    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_3D, m_volumeTexture );
    glUniform1iARB( m_gpuProgram->uniform( "uVolumeTexture" ), 1 );

    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_1D, m_transferFunctionTexture );
    glUniform1iARB( m_gpuProgram->uniform( "uTransferFunctionTexture" ), 2 );

    glCullFace( GL_BACK );

    drawVertexBufferObject();

    glUseProgramObjectARB( 0 );
}


void QGpuTestingViewer::drawVertexBufferObject()
{
    glEnableClientState( GL_NORMAL_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );
    glEnableClientState( GL_VERTEX_ARRAY );

    glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_vertexBufferObject );

    glNormalPointer( GL_FLOAT, sizeof( VertexBufferObjectData ), 0 );
    glColorPointer( 3, GL_FLOAT, sizeof( VertexBufferObjectData ), reinterpret_cast<const GLvoid*>( 3 * sizeof( GLfloat ) ) );
    glVertexPointer( 3, GL_FLOAT, sizeof( VertexBufferObjectData ), reinterpret_cast<const GLvoid*>( 6 * sizeof( GLfloat ) ) );

    glDrawArrays( GL_QUADS, 0, 24 );    // 24 = 4 vèrtexs/cara * 6 cares

    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );
}


}