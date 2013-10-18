/*==================================================================================
 Copyright (c) 2008, binaryzebra
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the binaryzebra nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 =====================================================================================*/

#include "LandscapeDialog.h"
#include <QLabel>
#include <QMessageBox>
#include "ui_BaseAddEntityDialog.h"
#include "SceneEditor/EditorSettings.h"
#include "Main/mainwindow.h"
#include "Classes/Commands2/EntityAddCommand.h"
#include "Classes/Commands2/EntityRemoveCommand.h"
#include "Classes/Commands2/LandscapeSetTexturesCommands.h"
#include "Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataDavaVariant.h"

#define  INIT_PATH_WIDGET(widgetName, widgetNum, widgetTitle, fileFilter) SelectPathWidgetBase* widgetName = new SelectPathWidgetBase(parent,resFolder,"", widgetTitle, fileFilter);\
	if(innerLandscape){\
		widgetName->setText(innerLandscape->GetTextureName(widgetNum).GetAbsolutePathname());\
		widgetMap[widgetName] = widgetNum;\
	}\
	else{\
		widgetMap[widgetName] = widgetNum;\
	}\
	widgetName->setEnabled(innerLandscape);\
	widgetName->SetAllowedFormatsList(textureFormats);\
	connect(widgetName, SIGNAL(PathSelected(DAVA::String)), this, SLOT(PathWidgetValueChanged(DAVA::String)));

#define OPEN_TEXTURE_TITLE "Open texture"
#define OPEN_HEIGHTMAP_TITLE "Open height map"
#define TEXTURE_FILE_FILTER "PNG(*.png);; TEX(*.tex)"
#define HEIGHTMAP_FILE_FILTER "PNG(*.png);;HeightMap(*.heightmap)"
#define HEIGHT_MAP_ID 0xABCD

#define TAB_CONTENT_WIDTH 450
#define TAB_CONTENT_HEIGHT 300

#define CREATE_TITLE "Create"
#define DELETE_TITLE "Delete"


#define DEFAULT_LANDSCAPE_SIDE_LENGTH	445.0f
#define DEFAULT_LANDSCAPE_HEIGHT		50.0f

LandscapeDialog::LandscapeDialog(Entity* _landscapeEntity,  QWidget* parent)
:BaseAddEntityDialog(parent)
{
	setWindowTitle("Landscape Settings");
	setAcceptDrops(true);
	setAttribute( Qt::WA_DeleteOnClose, true );
	DAVA::List<DAVA::String> textureFormats;
	textureFormats.push_back(".tex");
	textureFormats.push_back(".png");
	
	DAVA::List<DAVA::String> heightMapFormats;
	heightMapFormats.push_back(".heightmap");

	SetEntity(_landscapeEntity);
	innerLandscape = DAVA::GetLandscape(entity);

	connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(SceneActivated(SceneEditor2 *)));
	
	DAVA::String resFolder = EditorSettings::Instance()->GetDataSourcePath().GetAbsolutePathname();
	
	INIT_PATH_WIDGET(colorTexutureWidget, Landscape::TEXTURE_COLOR, OPEN_TEXTURE_TITLE, TEXTURE_FILE_FILTER);
	INIT_PATH_WIDGET(tileTexuture0Widget, Landscape::TEXTURE_TILE0, OPEN_TEXTURE_TITLE, TEXTURE_FILE_FILTER);
	INIT_PATH_WIDGET(tileTexuture1Widget, Landscape::TEXTURE_TILE1, OPEN_TEXTURE_TITLE, TEXTURE_FILE_FILTER);
	INIT_PATH_WIDGET(tileTexuture2Widget, Landscape::TEXTURE_TILE2, OPEN_TEXTURE_TITLE, TEXTURE_FILE_FILTER);
	INIT_PATH_WIDGET(tileTexuture3Widget, Landscape::TEXTURE_TILE3, OPEN_TEXTURE_TITLE, TEXTURE_FILE_FILTER);
	INIT_PATH_WIDGET(tileMaskWidget,	  Landscape::TEXTURE_TILE_MASK, OPEN_TEXTURE_TITLE, TEXTURE_FILE_FILTER);
	
	SelectPathWidgetBase* heightMapWidget = new SelectPathWidgetBase(parent, resFolder,"",
																	 OPEN_HEIGHTMAP_TITLE, HEIGHTMAP_FILE_FILTER);
	widgetMap[heightMapWidget] = HEIGHT_MAP_ID;
	if(innerLandscape)
	{
		heightMapWidget->setText(innerLandscape->GetHeightmapPathname().GetAbsolutePathname());
	}
	heightMapWidget->setEnabled(innerLandscape);
	heightMapWidget->SetAllowedFormatsList(heightMapFormats);
	connect(heightMapWidget, SIGNAL(PathSelected(DAVA::String)), this, SLOT(PathWidgetValueChanged(DAVA::String)));
	
	AddControlToUserContainer(heightMapWidget, "Height map:");
	AddControlToUserContainer(colorTexutureWidget, "Color texture:");
	AddControlToUserContainer(tileTexuture0Widget, "Tile texture 0:");
	AddControlToUserContainer(tileTexuture1Widget, "Tile texture 1:");
	AddControlToUserContainer(tileTexuture2Widget, "Tile texture 2:");
	AddControlToUserContainer(tileTexuture3Widget, "Tile texture 3:");
	AddControlToUserContainer(tileMaskWidget, "Tile mask:");
	
	QString btnTitle = CREATE_TITLE;
	if(innerLandscape != NULL)
	{
		TileModeChanged((int)innerLandscape->GetTiledShaderMode());
		btnTitle = DELETE_TITLE;
	}
	actionButton = new QPushButton(btnTitle, this);
	connect(actionButton, SIGNAL(clicked()), this, SLOT(ActionButtonClicked()));
	ui->lowerLayOut->removeWidget(ui->buttonBox);
	ui->lowerLayOut->addWidget(actionButton, 0, 0);
	ui->lowerLayOut->addWidget(ui->buttonBox, 0, 1);
	
	QtPropertyDataDavaVariant* sizePropertyDataVariant = NULL;
	QtPropertyDataDavaVariant* hightPropertyDataVariant = NULL;
	
	QObject::connect(SceneSignals::Instance(), SIGNAL(CommandExecuted(SceneEditor2 *, const Command2*, bool)), this, SLOT(CommandExecuted(SceneEditor2 *, const Command2*, bool )));
	landscapeSize = Vector3(445, 445, 50);//todo
}

LandscapeDialog::~LandscapeDialog()
{
	disconnect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2 *)), this, SLOT(SceneActivated(SceneEditor2 *)));
	
	for (DAVA::Map<SelectPathWidgetBase*,int32>::iterator it = widgetMap.begin(); it != widgetMap.end(); ++it)
	{
		delete it->first;
	}

	delete actionButton;
}

void LandscapeDialog::FillPropertyEditorWithContent()
{
	propEditor->RemovePropertyAll();
	if(NULL == entity)
	{
		return;
	}
	
	landscapeSize = GetSizeOfCurrentLandscape();
	AddMetaObject(&landscapeSize.x, DAVA::MetaInfo::Instance<float>(), "Size");
	AddMetaObject(&landscapeSize.z, DAVA::MetaInfo::Instance<float>(), "Height");

	DAVA::KeyedArchive* propertyList = entity->GetCustomProperties();
	if(!propertyList->IsKeyExists("lightmap.size"))
	{
		propertyList->SetInt32("lightmap.size", 1024);
	}
	if(!propertyList->IsKeyExists("editor.staticlight.enable"))
	{
		propertyList->SetBool("editor.staticlight.enable", false);
	}
	if(!propertyList->IsKeyExists("editor.staticlight.castshadows"))
	{
		propertyList->SetBool("editor.staticlight.castshadows", false);
	}
	if(!propertyList->IsKeyExists("editor.staticlight.receiveshadows"))
	{
		propertyList->SetBool("editor.staticlight.receiveshadows", false);
	}

	AddKeyedArchiveMember(propertyList, "lightmap.size", "Lightmap size");
	AddKeyedArchiveMember(propertyList, "editor.staticlight.enable", "Staticlight enable");
	AddKeyedArchiveMember(propertyList, "editor.staticlight.castshadows", "Cast shadows"); 
	AddKeyedArchiveMember(propertyList, "editor.staticlight.receiveshadows", "Receive shadows");

	AddInspMemberToEditor( innerLandscape, innerLandscape->GetTypeInfo()->Member("isFogEnabled"));
	AddInspMemberToEditor( innerLandscape, innerLandscape->GetTypeInfo()->Member("fogDensity"));
	AddInspMemberToEditor( innerLandscape, innerLandscape->GetTypeInfo()->Member("fogColor"));
	QtPropertyData* tileMode = AddInspMemberToEditor( innerLandscape, innerLandscape->GetTypeInfo()->Member("tiledShaderMode"));
	QtPropertyDataDavaVariant* tileModeVariant = dynamic_cast<QtPropertyDataDavaVariant*>(tileMode);

	Vector<String> tiledModes;
	tiledModes.push_back("Tile mask mode");
	tiledModes.push_back("Texture mode");
	tiledModes.push_back("Mixed mode");
	tiledModes.push_back("Detail mask mode");
	uint32 shaderMode = innerLandscape->GetTiledShaderMode();
	String sMode =  tiledModes[shaderMode];
	tileModeVariant->AddAllowedValue(DAVA::VariantType(shaderMode), sMode.c_str());
	if(shaderMode != Landscape::TILED_MODE_TILE_DETAIL_MASK)
	{
		tileModeVariant->AddAllowedValue(DAVA::VariantType((uint32)Landscape::TILED_MODE_TILE_DETAIL_MASK), "Detail mask mode");
	}

	AddInspMemberToEditor( innerLandscape, innerLandscape->GetTypeInfo()->Member("tileColor"));
	AddInspMemberToEditor( innerLandscape, innerLandscape->GetTypeInfo()->Member("textureTiling"));
}

void LandscapeDialog::FillWidgetsWithContent()
{
	Landscape*	landscapeToProcess =  entity == NULL ? NULL : DAVA::GetLandscape(entity);
	
	for (DAVA::Map<SelectPathWidgetBase*,int32>::iterator it = widgetMap.begin(); it != widgetMap.end(); ++it)
	{
		SelectPathWidgetBase* widget = it->first;
		int32 info = it->second;
		
		if(landscapeToProcess)
		{
			if(info == HEIGHT_MAP_ID)
			{
				widget->setText(landscapeToProcess->GetHeightmapPathname().GetAbsolutePathname());
			}
			else
			{
				String t = landscapeToProcess->GetTextureName((Landscape::eTextureLevel)info).GetAbsolutePathname();
				widget->setText(landscapeToProcess->GetTextureName((Landscape::eTextureLevel)info).GetAbsolutePathname());
			}
		}
		else
		{
			widget->blockSignals(true);
			widget->setText(DAVA::String(""));
			widget->blockSignals(false);
		}
		
		widget->setEnabled(landscapeToProcess);
	}
	
	if(landscapeToProcess != NULL)
	{
		actionButton->setText(DELETE_TITLE);
		TileModeChanged((int)landscapeToProcess->GetTiledShaderMode());
	}
	else
	{
		actionButton->setText(CREATE_TITLE);
	}
}

void LandscapeDialog::showEvent ( QShowEvent * event )
{
	BaseAddEntityDialog::showEvent(event);
	SetLandscapeEntity(entity);
}

void LandscapeDialog::CommandExecuted(SceneEditor2 *scene, const Command2* command, bool redo)
{
	int id = command->GetId() ;
	if( id == CMDID_ENTITY_ADD || id == CMDID_ENTITY_REMOVE)
	{
		Entity* commandEntity = command->GetEntity();
		Entity* landscapeEntity = FindLandscapeEntity(scene);
		bool isEqual = commandEntity == landscapeEntity;
		
		if(id == CMDID_ENTITY_ADD)
		{
				if(!redo)
				{
					SetLandscapeEntity(NULL);
				}
				else if (redo && isEqual)
				{
					SetLandscapeEntity(commandEntity);
				}
		}
		else
		{
				if(!redo && isEqual)
				{
					SetLandscapeEntity(commandEntity);
				}
				else if (redo)
				{
					SetLandscapeEntity(NULL);
				}
		}
	}
	//CMDID_LANDSCAPE_SET_TEXTURE
	//CMDID_LANDSCAPE_SET_HEIGHTMAP
	if(id == CMDID_LANDSCAPE_SET_TEXTURE || id == CMDID_LANDSCAPE_SET_HEIGHTMAP )
	{
		FillWidgetsWithContent();
	}

	propEditor->Update();
}

void LandscapeDialog::ActionButtonClicked()
{
	SceneEditor2* sceneEditor = QtMainWindow::Instance()->GetCurrentScene();
	bool needToCreate = actionButton->text() == CREATE_TITLE;
	if(needToCreate)
	{
		actionButton->setText(DELETE_TITLE);
		
		Entity* entityToProcess = new Entity();
		entityToProcess->SetName(ResourceEditor::LANDSCAPE_NODE_NAME);
		Landscape* newLandscape = new Landscape();
		newLandscape->SetTiledShaderMode(Landscape::TILED_MODE_TILE_DETAIL_MASK);
		RenderComponent* component = new RenderComponent(ScopedPtr<Landscape>(newLandscape));
		entityToProcess->AddComponent(component);
		SafeRelease(component);

		AABBox3 bboxForLandscape;
		bboxForLandscape.AddPoint(Vector3(-landscapeSize.x/2.f, -landscapeSize.y/2.f, 0.f));
		bboxForLandscape.AddPoint(Vector3(landscapeSize.x/2.f, landscapeSize.y/2.f, landscapeSize.z));
		newLandscape->BuildLandscapeFromHeightmapImage("", bboxForLandscape);

		EntityAddCommand* commandAdd = new EntityAddCommand(entityToProcess, sceneEditor);
		sceneEditor->Exec(commandAdd);
		sceneEditor->selectionSystem->SetSelection(entityToProcess);

		SetLandscapeEntity(entityToProcess);//BaseAddEntityDialog::entity is inited
		SafeRelease(entityToProcess);
	}
	else
	{
		actionButton->setText(CREATE_TITLE);
		
		if(entity != NULL)
		{
			EntityRemoveCommand * command = new EntityRemoveCommand(entity);
			sceneEditor->Exec(command);
		}

		SetLandscapeEntity(NULL);
	}
}

void LandscapeDialog::OnItemEdited(const QString &name, QtPropertyData *data)
{
	SceneEditor2 *curScene = QtMainWindow::Instance()->GetCurrentScene();
	if(NULL == curScene)
	{
		return;		
	}

	bool isMultiple = "Size" == name || "Height" == name;
	
	if(isMultiple)
	{
		curScene->BeginBatch("Landscape resizing");
	}
	
	Command2 *command = (Command2 *) data->CreateLastCommand();
	if(NULL != command)
	{
		curScene->Exec(command);
	}
	
	if("Size" == name || "Height" == name)
	{
		landscapeSize.y = landscapeSize.x;
		AABBox3 bboxForLandscape;
		bboxForLandscape.AddPoint(Vector3(-landscapeSize.x/2.f, -landscapeSize.y/2.f, 0.f));
		bboxForLandscape.AddPoint(Vector3(landscapeSize.x/2.f, landscapeSize.y/2.f, landscapeSize.z));
		LandscapeSetHeightMapCommand* command = new LandscapeSetHeightMapCommand(entity,  innerLandscape->GetHeightmapPathname(), bboxForLandscape);
		curScene->Exec(command);
		
		curScene->EndBatch();
	}
	
	if( "tiledShaderMode" == name )
	{
		int newTileMode = data->GetValue().toInt();
		TileModeChanged(newTileMode);
	}
}

Vector3 LandscapeDialog::GetSizeOfCurrentLandscape()
{
	Vector3 retValue;
	if(NULL == innerLandscape)
	{
		return retValue;
	}
	DAVA::AABBox3 bbox = innerLandscape->GetBoundingBox();
	if(!bbox.GetSize().IsZero())
	{
		DAVA::AABBox3 transformedBox = bbox;
		if(NULL != innerLandscape->GetWorldTransformPtr())
		{
			bbox.GetTransformedBox(*innerLandscape->GetWorldTransformPtr(), transformedBox);
		}
		retValue = transformedBox.max - transformedBox.min;
	}
	return retValue;
}

void LandscapeDialog::SetLandscapeEntity(Entity* _landscapeEntity)
{
	SetEntity(_landscapeEntity);
	innerLandscape = _landscapeEntity == NULL ? NULL : DAVA::GetLandscape(_landscapeEntity);
	FillUIbyLandscapeEntity();
}

void LandscapeDialog::FillUIbyLandscapeEntity()
{
	FillWidgetsWithContent();
	FillPropertyEditorWithContent();
	propEditor->expandAll();
	PerformResize();
}

void LandscapeDialog::TileModeChanged(int newValue)
{
	bool shouldEnableControls = (Landscape::eTiledShaderMode )newValue != Landscape::TILED_MODE_TILE_DETAIL_MASK;

	SelectPathWidgetBase* tileTexuture0Widget = FindWidgetBySpecInfo(Landscape::TEXTURE_TILE0);
	SelectPathWidgetBase* tileTexuture1Widget = FindWidgetBySpecInfo(Landscape::TEXTURE_TILE1);
	SelectPathWidgetBase* tileTexuture2Widget = FindWidgetBySpecInfo(Landscape::TEXTURE_TILE2);
	SelectPathWidgetBase* tileTexuture3Widget = FindWidgetBySpecInfo(Landscape::TEXTURE_TILE3);
	
	tileTexuture1Widget->setEnabled(shouldEnableControls);
	tileTexuture2Widget->setEnabled(shouldEnableControls);
	tileTexuture3Widget->setEnabled(shouldEnableControls);
		
	if(!shouldEnableControls)
	{
		tileTexuture1Widget->blockSignals(true);
		tileTexuture2Widget->blockSignals(true);
		tileTexuture3Widget->blockSignals(true);
		
		DAVA::String path = tileTexuture0Widget->getText();
		tileTexuture1Widget->setText(path);
		tileTexuture2Widget->setText(path);
		tileTexuture3Widget->setText(path);
		
		tileTexuture1Widget->blockSignals(false);
		tileTexuture2Widget->blockSignals(false);
		tileTexuture3Widget->blockSignals(false);
	}
}

SelectPathWidgetBase* LandscapeDialog::FindWidgetBySpecInfo(int value)
{
	SelectPathWidgetBase* retValue = NULL;
	for(DAVA::Map<SelectPathWidgetBase*,int32>::iterator it = widgetMap.begin(); it != widgetMap.end();++it)
	{
		if(it->second == value)
		{
			retValue = it->first;
			break;
		}
	}
	return retValue;
}

void LandscapeDialog::CleanupPathWidgets()
{
	for (DAVA::Map<SelectPathWidgetBase*,int32>::iterator it = widgetMap.begin(); it != widgetMap.end(); ++it)
	{
		it->first->EraseWidget();
	}
}

void LandscapeDialog::PathWidgetValueChanged(String fileName)
{
	SceneEditor2 *sceneEditor = QtMainWindow::Instance()->GetCurrentScene();
	if(!innerLandscape)
	{
		return;
	}
	SelectPathWidgetBase* sender = dynamic_cast<SelectPathWidgetBase*>(QObject::sender());
	
	FilePath filePath(fileName);
	if(widgetMap[sender] == HEIGHT_MAP_ID)
	{
		FilePath presentPath = innerLandscape->GetHeightmapPathname();
		if(filePath != presentPath)
		{
			LandscapeSetHeightMapCommand* command = new LandscapeSetHeightMapCommand(entity, filePath, innerLandscape->GetBoundingBox());
			sceneEditor->Exec(command);
		}
	}
	else
	{
		int id = widgetMap[sender];
		FilePath presentName = innerLandscape->GetTextureName((Landscape::eTextureLevel)id);
		if(filePath != presentName)
		{

			LandscapeSetTexturesCommand* command = new LandscapeSetTexturesCommand(entity, (Landscape::eTextureLevel)id, filePath);
			sceneEditor->Exec(command);

			if(innerLandscape->GetTiledShaderMode() == Landscape::TILED_MODE_TILE_DETAIL_MASK)
			{
				TileModeChanged(Landscape::TILED_MODE_TILE_DETAIL_MASK);
			}
		}
	}
}

void LandscapeDialog::SceneActivated(SceneEditor2 *editor)
{
	SetLandscapeEntity(FindLandscapeEntity(editor));
}
