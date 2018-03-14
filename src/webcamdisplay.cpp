/*
 * Copyright 2015-2017 Markus Prasser
 *
 * This file is part of WebcamDisplay.
 *
 *  WebcamDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  WebcamDisplay is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with WebcamDisplay.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "webcamdisplay.h"
#include "ui_webcamdisplay.h"
#include "ui_authenticationdialog.h"

#include <QAuthenticator>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>

MWWebcamDisplay::MWWebcamDisplay(const QString &argWebcamURL,
                                 QWidget *argParent) :
    QMainWindow{argParent},
    ui{new Ui::MWWebcamDisplay},
webcamURL{new QUrl{argWebcamURL}}
{
    qnam = new QNetworkAccessManager{this};

    ui->setupUi(this);
    scene = new QGraphicsScene{this};
    ui->GVImageDisplay->setScene(scene);
    this->setWindowTitle(tr("WebcamDisplay"));
            // Do not show Webcam string
            // + webcamURL->toString());

    connect(qnam, &QNetworkAccessManager::authenticationRequired,
            this, &MWWebcamDisplay::AuthenticationRequired);
    connect(qnam, &QNetworkAccessManager::sslErrors,
            this, &MWWebcamDisplay::SSLErrors);

    refreshTimer = new QTimer{this};
    connect(refreshTimer, &QTimer::timeout,
            this, &MWWebcamDisplay::StartRequest);
    refreshTimer->start(1024);
}

MWWebcamDisplay::~MWWebcamDisplay()
{
    if (currentImage) {
        scene->removeItem(currentImage.get());
    }
    delete ui;
}

void MWWebcamDisplay::AuthenticationRequired(QNetworkReply *argReply,
                                             QAuthenticator *argAuthenticator)
{
    Q_UNUSED(argReply)

    QDialog dialog;
    Ui::Dialog ui;
    ui.setupUi(&dialog);
    ui.LSiteDescription->setText(tr("%1 at %2").arg(argAuthenticator->realm(), webcamURL->host()));

    ui.LEUsername->setText(webcamURL->userName());
    ui.LEPassword->setText(webcamURL->password());

    if (dialog.exec() == QDialog::Accepted) {
        argAuthenticator->setUser(ui.LEUsername->text());
        argAuthenticator->setPassword(ui.LEPassword->text());
    }
}

void MWWebcamDisplay::HttpFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error()) {
        // QDebug
        // QMessageBox::information(this, tr("HTTP"),
        //                         tr("Download failed: %1.").arg(reply->errorString()));
    } else if (!redirectionTarget.isNull()) {
        const QUrl newURL = webcamURL->resolved(redirectionTarget.toUrl());
        if (QMessageBox::question(this, tr("HTTP"),
                                  tr("Redirect to '%1'?").arg(newURL.toString()),
                                  QMessageBox::No | QMessageBox::Yes)
                == QMessageBox::Yes) {
            webcamURL.reset(new QUrl{newURL});
            reply->deleteLater();
            reply = nullptr;
            return;
        }
    }

    const QByteArray replyByteArray{reply->readAll()};
    reply->deleteLater();
    reply = nullptr;

    QPixmap image;
    image.loadFromData(replyByteArray);
    recentImage.reset(currentImage.release());
    currentImage.reset(scene->addPixmap(image));
    if (recentImage) {
        scene->removeItem(recentImage.get());
    }
    ui->GVImageDisplay->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MWWebcamDisplay::SSLErrors(QNetworkReply *argReply,
                                const QList<QSslError> &errors)
{
    QString errorString;
    for (auto cit = errors.constBegin(); cit != errors.constEnd(); ++cit) {
        if (!errorString.isEmpty()) {
            errorString += "\n";
        }
        errorString += cit->errorString();
    }

    if (QMessageBox::warning(this, tr("HTTP"),
                             tr("One or more SSL errors occurred:\n%1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort)
            == QMessageBox::Ignore) {
        argReply->ignoreSslErrors();
    }
}

void MWWebcamDisplay::StartRequest()
{
    QNetworkReply *const reply = qnam->get(QNetworkRequest{*webcamURL});

    connect(reply, &QNetworkReply::finished,
            this, &MWWebcamDisplay::HttpFinished);
}
