/*-
 * Copyright (c) 2012 Oleksandr Tymoshenko <gonzo@bluezbox.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "logowindow.h"

#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>

#include <QtCore/qmath.h>
#include <QTimer>

Renderer::Renderer(const QSurfaceFormat &format)
    : 
    m_initialized(false)
    , m_format(format)
    , m_program(0)
    , m_frame(0)
    , m_surface(0)
{
    m_context = new QOpenGLContext(this);
    m_context->setFormat(format);
    m_context->create();

    m_backgroundColor = QColor::fromRgbF(0.1f, 0.1f, 0.2f, 1.0f);
}

void Renderer::setAnimating(LogoWindow *window, bool animating)
{
    if (animating) {
        m_surface = window;
        QTimer::singleShot(0, this, SLOT(render()));
    }
    else {
        m_surface = NULL;
    }
}

void Renderer::initialize()
{
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl");
    m_program->link();
    m_program->bind();

    vertexAttr = m_program->attributeLocation("vertex");
    normalAttr = m_program->attributeLocation("normal");
    matrixUniform = m_program->uniformLocation("matrix");
    colorUniform = m_program->uniformLocation("sourceColor");

    createGeometry();

    m_vbo.create();
    m_vbo.bind();
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    m_vbo.allocate(verticesSize * 2);
    m_vbo.write(0, vertices.constData(), verticesSize);
    m_vbo.write(verticesSize, normals.constData(), verticesSize);
    m_vbo.release();

    m_program->release();
}

void Renderer::render()
{
    if (!m_surface)
        return;

    if (!m_context->makeCurrent(m_surface))
        return;

    if (!m_initialized) {
        initialize();
        m_initialized = true;
    }

    QSize viewSize = m_surface->size();

    QOpenGLFunctions *f = m_context->functions();
    f->glViewport(0, 0, viewSize.width() * m_surface->devicePixelRatio(), viewSize.height() * m_surface->devicePixelRatio());
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), m_backgroundColor.alphaF());
    // f->glFrontFace(GL_CW);
    // f->glCullFace(GL_FRONT);
    // f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_DEPTH_TEST);

    m_program->bind();
    m_vbo.bind();

    m_program->enableAttributeArray(vertexAttr);
    m_program->enableAttributeArray(normalAttr);
    m_program->setAttributeBuffer(vertexAttr, GL_FLOAT, 0, 3);
    const int verticesSize = vertices.count() * 3 * sizeof(GLfloat);
    m_program->setAttributeBuffer(normalAttr, GL_FLOAT, verticesSize, 3);

    QMatrix4x4 modelview;
    modelview.rotate(90, -1.0f, 0.0f, 0.0f);
    modelview.rotate((float)m_frame, 0.0f, 0.0f, 1.0f);
    // modelview.rotate((float)m_frame, 1.0f, 0.0f, 0.0f);
    // modelview.rotate((float)m_frame, 0.0f, 0.0f, 1.0f);
    // modelview.translate(0.0f, -0.2f, 0.0f);

    m_program->setUniformValue(matrixUniform, modelview);
    m_program->setUniformValue(colorUniform, QColor(200, 0, 0, 255));

    f->glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    m_context->swapBuffers(m_surface);

    ++m_frame;

    // m_vbo.release();
    // m_program->release();
    QTimer::singleShot(0, this, SLOT(render()));
}

void Renderer::createGeometry()
{
    vertices.clear();
    normals.clear();

    createSphere();
    createHorns();

    for (int i = 0;i < vertices.size();i++)
        vertices[i] *= 2.0f;
}

void Renderer::createSphere()
{
    const qreal Pi = 3.14159f;
    const qreal r = 0.30;
    const int NumSectors = 200;

    // QMatrix4x4 rotation;
    // rotation.rotate(90, 1.0f, 0.0f, 0.0f);

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle1 = (i * 2 * Pi) / NumSectors;
        qreal angle2 = ((i + 1) * 2 * Pi) / NumSectors;

        for (int j = 0; j < NumSectors/2; ++j) {
            qreal angle3 = (j * 2 * Pi) / NumSectors;
            qreal angle4 = ((j + 1) * 2 * Pi) / NumSectors;
            QVector3D p1 = fromSph(r, angle1, angle3);
            QVector3D p2 = fromSph(r, angle1, angle4);
            QVector3D p3 = fromSph(r, angle2, angle4);
            QVector3D p4 = fromSph(r, angle2, angle3);

            // p1 = p1 * rotation;
            // p2 = p2 * rotation;
            // p3 = p3 * rotation;
            // p4 = p4 * rotation;

            QVector3D n = QVector3D::normal(p1 - p2, p3 - p2);
            if (j + 1 == NumSectors/2) // p2 == p3
                n = QVector3D::normal(p1 - p4, p1 - p2);

            vertices << p1;
            vertices << p2;
            vertices << p3;

            vertices << p3;
            vertices << p4;
            vertices << p1;

            normals << n;
            normals << n;
            normals << n;

            normals << n;
            normals << n;
            normals << n;
        }
    }
}

void Renderer::createHorns()
{
    QMatrix4x4 transform1;
    transform1.translate(-0.3, 0, 0.3);
    transform1.rotate(135, 0.0f, 1.0f, 0.0f);
    transform1.scale(0.3, 0.3, 0.3);

    createHorn(transform1);

    QMatrix4x4 transform2;
    transform2.translate(0.3, 0, 0.3);
    transform2.rotate(225, 0.0f, 1.0f, 0.0f);
    transform2.scale(0.3, 0.3, 0.3);

    createHorn(transform2);
}

void Renderer::createHorn(QMatrix4x4 transform, int details)
{
    const qreal Pi = 3.14159f;
    const int NumSectors = details;
    const qreal a = 7;

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle1 = (i * 2 * Pi) / NumSectors;
        qreal angle2 = ((i + 1) * 2 * Pi) / NumSectors;

        qreal r = 0;
        const qreal r_step = 0.01;
        while ( r*r*a < 0.5) {
            qreal r1 = r;
            qreal r2 = r + r_step;
            r += r_step;

            QVector3D p1(r1*qSin(angle1), r1*qCos(angle1), r1*r1*a);
            QVector3D p2(r2*qSin(angle1), r2*qCos(angle1), r2*r2*a);
            QVector3D p3(r2*qSin(angle2), r2*qCos(angle2), r2*r2*a);
            QVector3D p4(r1*qSin(angle2), r1*qCos(angle2), r1*r1*a);

            p1 = transform.map(p1);
            p2 = transform.map(p2);
            p3 = transform.map(p3);
            p4 = transform.map(p4);

            QVector3D n;
            if (r1 == 0) {
                QVector3D d1 = p1 - p2;
                QVector3D d2 = p3 - p1;
                d1.normalize();
                d2.normalize();
                n = QVector3D::normal(d1, d2);
            }
            else
                n = QVector3D::normal(p1 - p2, p3 - p2);

            vertices << p1;
            vertices << p2;
            vertices << p3;

            vertices << p3;
            vertices << p4;
            vertices << p1;

            normals << n;
            normals << n;
            normals << n;

            normals << n;
            normals << n;
            normals << n;
        }
    }
}

QVector3D Renderer::fromSph(qreal r, qreal theta, qreal phi)
{
    return QVector3D(r*qCos(theta)*qSin(phi),
        r*qSin(theta)*qSin(phi), r*qCos(phi));
}

LogoWindow::LogoWindow(Renderer *renderer)
    : m_renderer(renderer)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    setGeometry(QRect(10, 10, 640, 640));

    setFormat(renderer->format());
    create();
}

void LogoWindow::exposeEvent(QExposeEvent *)
{
    m_renderer->setAnimating(this, isExposed());
}