/*
    Quickddit - Reddit client for mobile phones
    Copyright (C) 2015  Sander van Grieken

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see [http://www.gnu.org/licenses/].
*/

#include <QDebug>
#include <QtNetwork/QNetworkReply>

#include "linkmanager.h"
#include "linkmodel.h"
#include "userthingmodel.h"
#include "parser.h"

LinkManager::LinkManager(QObject *parent) :
    AbstractManager(parent), m_linkModel(0), m_commentModel(0), m_userThingModel(0)
{
}

LinkModel *LinkManager::linkModel() const
{
    return m_linkModel;
}

void LinkManager::setLinkModel(LinkModel *model)
{
    m_linkModel = model;
}

CommentModel *LinkManager::commentModel() const
{
    return m_commentModel;
}

void LinkManager::setCommentModel(CommentModel *model)
{
    m_commentModel = model;
}

UserThingModel *LinkManager::userThingModel() const
{
    return m_userThingModel;
}

void LinkManager::setUserThingModel(UserThingModel *model)
{
    m_userThingModel = model;
}

void LinkManager::submit(const QString &subreddit, const QString &title, const QString& url, const QString& text, const QString& flairId)
{
    m_action = Submit;

    QHash<QString, QString> parameters;
    parameters.insert("api_type", "json");
    // self?
    parameters.insert("kind", url.isEmpty() ? "self" : "link");
    if (!url.isEmpty())
        parameters.insert("url", url);
    if (!flairId.isEmpty())
        parameters.insert("flair_id", flairId);
    // basic
    parameters.insert("sr", subreddit);
    parameters.insert("text", text);
    parameters.insert("title", title);

    doRequest(APIRequest::POST, "/api/submit", SLOT(onFinished(QNetworkReply*)), parameters);
}

void LinkManager::editLinkText(const QString &fullname, const QString &rawText)
{
    m_action = Edit;

    QHash<QString, QString> parameters;
    parameters.insert("api_type", "json");
    parameters.insert("text", rawText);
    parameters.insert("thing_id", fullname);
    m_fullname = fullname;
    m_text = rawText;

    doRequest(APIRequest::POST, "/api/editusertext", SLOT(onFinished(QNetworkReply*)), parameters);
}

void LinkManager::deleteLink(const QString &fullname)
{
    m_action = Delete;

    QHash<QString, QString> parameters;
    parameters.insert("api_type", "json");
    parameters.insert("id", fullname);
    m_fullname = fullname;

    doRequest(APIRequest::POST, "/api/del", SLOT(onFinished(QNetworkReply*)), parameters);
}

void LinkManager::hideLink(const QString &fullname)
{
    m_action = Delete;

    QHash<QString, QString> parameters;
    parameters.insert("api_type", "json");
    parameters.insert("id", fullname);
    m_fullname = fullname;

    doRequest(APIRequest::POST, "/api/hide", SLOT(onFinished(QNetworkReply*)), parameters);
}

void LinkManager::onFinished(QNetworkReply *reply)
{
    if (reply != 0) {
        if (reply->error() == QNetworkReply::NoError) {
            if (m_action == Submit) {
                QList<QString> errors = Parser::parseErrors(reply->readAll());
                if (errors.size() == 0) {
                    emit success(tr("The link has been added"));
                } else {
                    emit error("Error:" + errors.first());
                }
            } else if (m_action == Edit) {
                LinkObject link = Parser::parseLinkEditResponse(reply->readAll());
                emit success(tr("The link text has been changed"));
                // update both occurrences of a Link. Bit ugly to have to refer to two models,
                // but this way the UI never shows stale data.
                if (m_linkModel)
                    m_linkModel->editLink(link);
                if (m_commentModel)
                    m_commentModel->setLink(LinkModel::toLinkVariantMap(link));
            } else {
                if (m_userThingModel)
                    m_userThingModel->deleteThing(m_fullname);
                if (m_linkModel)
                    m_linkModel->deleteLink(m_fullname);
                qDebug() << "delete" << reply->readAll();
            }
        } else {
            emit error(reply->errorString());
        }
    }
}
