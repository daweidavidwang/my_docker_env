/*
 * Copyright (C) 2015-2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/GLWidget.hh"

#include "gazebo/gui/ApplyWrenchDialog.hh"
#include "gazebo/gui/ApplyWrenchDialog_TEST.hh"

#include "test_config.h"

/////////////////////////////////////////////////
void ApplyWrenchDialog_TEST::ApplyForceTorqueFromDialog()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  // World with one model which has 2 links, no ground plane and gravity is off
  this->Load("worlds/multilink_shape.world", false, false, false);

  // Create the main window.
  gazebo::gui::MainWindow *mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != nullptr);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  this->ProcessEventsAndDraw(mainWindow);

  // Get the user camera and scene
  gazebo::rendering::UserCameraPtr cam = gazebo::gui::get_active_camera();
  QVERIFY(cam != nullptr);
  gazebo::rendering::ScenePtr scene = cam->GetScene();
  QVERIFY(scene != nullptr);

  // Get the model
  gazebo::rendering::VisualPtr modelVis = scene->GetVisual("multilink");
  QVERIFY(modelVis != nullptr);

  // Get the box link
  gazebo::rendering::VisualPtr boxLinkVis =
      scene->GetVisual("multilink::box_link");
  QVERIFY(boxLinkVis != nullptr);
  auto boxLinkPose = boxLinkVis->GetWorldPose().Ign();
  QVERIFY(boxLinkPose == boxLinkVis->GetWorldPose().Ign());

  // Get the sphere link
  gazebo::rendering::VisualPtr sphereLinkVis =
      scene->GetVisual("multilink::sphere_link");
  QVERIFY(sphereLinkVis != nullptr);
  auto sphereLinkPose = sphereLinkVis->GetWorldPose().Ign();
  QVERIFY(sphereLinkPose == sphereLinkVis->GetWorldPose().Ign());

  // Check that an inexistent model doesn't break anything
  gazebo::gui::ApplyWrenchDialog *applyWrenchDialogFakeModel =
      new gazebo::gui::ApplyWrenchDialog();
  applyWrenchDialogFakeModel->Init("fake_model", "fake_link");

  // Check that an inexistent link doesn't break anything
  gazebo::gui::ApplyWrenchDialog *applyWrenchDialogFakeLink =
      new gazebo::gui::ApplyWrenchDialog();
  applyWrenchDialogFakeLink->Init("multilink", "fake_link");

  // Initialize dialog with the box link
  gazebo::gui::ApplyWrenchDialog *applyWrenchDialog =
      new gazebo::gui::ApplyWrenchDialog();
  applyWrenchDialog->Init("multilink", "multilink::box_link");

  // Get combo box
  QList<QComboBox *> comboBoxes =
      applyWrenchDialog->findChildren<QComboBox *>();
  QVERIFY(comboBoxes.size() == 1u);

  // Check the combo box's items
  QVERIFY(comboBoxes[0]->count() == 2u);
  QVERIFY(comboBoxes[0]->itemText(0) == "box_link");
  QVERIFY(comboBoxes[0]->itemText(1) == "sphere_link");
  QVERIFY(comboBoxes[0]->currentIndex() == 0u);

  // Get radio buttons
  QList<QRadioButton *> radioButtons =
      applyWrenchDialog->findChildren<QRadioButton *>();
  QVERIFY(radioButtons.size() == 2u);

  // Get spins
  QList<QDoubleSpinBox *> spins =
      applyWrenchDialog->findChildren<QDoubleSpinBox *>();
  QVERIFY(spins.size() == 11u);

  // Get buttons
  QList<QPushButton *> buttons =
      applyWrenchDialog->findChildren<QPushButton *>();
  QVERIFY(buttons.size() == 6u);

  QPushButton *applyForceButton = nullptr;
  QPushButton *applyTorqueButton = nullptr;
  QPushButton *applyAllButton = nullptr;
  QPushButton *clearForceButton = nullptr;
  QPushButton *clearTorqueButton = nullptr;
  QPushButton *cancelButton = nullptr;
  for (auto it : buttons)
  {
    QVERIFY(it);
    if (it->text().toLower().toStdString() == "apply force")
      applyForceButton = it;
    else if (it->text().toLower().toStdString() == "apply torque")
      applyTorqueButton = it;
    else if (it->text().toLower().toStdString() == "apply all")
      applyAllButton = it;
    else if (it->text().toLower().toStdString() == "clear" && !clearForceButton)
      clearForceButton = it;
    else if (it->text().toLower().toStdString() == "clear")
      clearTorqueButton = it;
    else if (it->text().toLower().toStdString() == "cancel")
      cancelButton = it;
  }
  QVERIFY(applyForceButton);
  QVERIFY(applyTorqueButton);
  QVERIFY(applyAllButton);
  QVERIFY(clearForceButton);
  QVERIFY(clearTorqueButton);
  QVERIFY(cancelButton);

  // Set and apply force on X axis, magnitude 1000
  spins[0]->setValue(1.0);
  spins[3]->setValue(1000.0);
  applyForceButton->click();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that force spin was updated according to magnitude
  QCOMPARE(spins[0]->value(), 1000.0);

  // Check that link moved on X axis
  QVERIFY(boxLinkPose.Pos().X() < boxLinkVis->GetWorldPose().Ign().Pos().X());
  QVERIFY(boxLinkPose.Pos().Y() -
      boxLinkVis->GetWorldPose().Ign().Pos().Y() < 1e-6);
  QVERIFY(boxLinkPose.Pos().Z() -
      boxLinkVis->GetWorldPose().Ign().Pos().Z() < 1e-6);
  QCOMPARE(boxLinkPose.Rot(), boxLinkVis->GetWorldPose().Ign().Rot());

  // Save current pose
  boxLinkPose = boxLinkVis->GetWorldPose().Ign();

  // Set and apply torque about -Z axis, magnitude 1000
  spins[9]->setValue(-1.0);
  spins[10]->setValue(1000.0);
  applyTorqueButton->click();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that torque spin was updated according to magnitude
  QCOMPARE(spins[9]->value(), -1000.0);

  // Check that link rotated
  QVERIFY(boxLinkPose.Pos().X() < boxLinkVis->GetWorldPose().Ign().Pos().X());
  QVERIFY(boxLinkPose.Pos().Y() -
      boxLinkVis->GetWorldPose().Ign().Pos().Y() < 1e-6);
  QVERIFY(boxLinkPose.Pos().Z() -
      boxLinkVis->GetWorldPose().Ign().Pos().Z() < 1e-6);
  QVERIFY(boxLinkPose.Rot() != boxLinkVis->GetWorldPose().Ign().Rot());

  // Save current pose
  boxLinkPose = boxLinkVis->GetWorldPose().Ign();

  // Apply force and torque
  applyAllButton->click();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that link translated and rotated
  QVERIFY(boxLinkPose.Pos() != boxLinkVis->GetWorldPose().Ign().Pos());
  QVERIFY(boxLinkPose.Rot() != boxLinkVis->GetWorldPose().Ign().Rot());

  // Change link
  comboBoxes[0]->setCurrentIndex(1);
  QVERIFY(comboBoxes[0]->currentText() == "sphere_link");

  // Clear force
  clearForceButton->click();
  QCOMPARE(spins[0]->value(), 0.0);
  QCOMPARE(spins[1]->value(), 0.0);
  QCOMPARE(spins[2]->value(), 0.0);
  QCOMPARE(spins[3]->value(), 0.0);

  // Clear torque
  clearTorqueButton->click();
  QCOMPARE(spins[7]->value(), 0.0);
  QCOMPARE(spins[8]->value(), 0.0);
  QCOMPARE(spins[9]->value(), 0.0);
  QCOMPARE(spins[10]->value(), 0.0);

  // Apply zero force and torque
  applyAllButton->click();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that link didn't move
  QVERIFY(sphereLinkPose.Pos() == sphereLinkVis->GetWorldPose().Ign().Pos());
  QVERIFY(sphereLinkPose.Rot() == sphereLinkVis->GetWorldPose().Ign().Rot());

  // Set and apply force on Y axis with an offset on X
  spins[1]->setValue(1000.0);
  spins[4]->setValue(1.0);
  applyForceButton->click();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that link translated and rotated
  QVERIFY(sphereLinkPose.Pos() != sphereLinkVis->GetWorldPose().Ign().Pos());
  QVERIFY(sphereLinkPose.Rot() != sphereLinkVis->GetWorldPose().Ign().Rot());

  // Select CoM as application point
  radioButtons[0]->click();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that force offset spins were updated
  QCOMPARE(spins[4]->value(), 0.0);
  QCOMPARE(spins[5]->value(), 0.0);
  QCOMPARE(spins[6]->value(), 0.0);

  // Close dialog
  cancelButton->click();

  this->ProcessEventsAndDraw(mainWindow);

  delete applyWrenchDialog;

  cam->Fini();
  mainWindow->close();
  delete mainWindow;
}

/////////////////////////////////////////////////
void ApplyWrenchDialog_TEST::MouseInteractions()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  // World with one model which has 2 links, no ground plane and gravity is off
  this->Load("worlds/multilink_shape.world", false, false, false);

  // Create the main window.
  gazebo::gui::MainWindow *mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != nullptr);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  this->ProcessEventsAndDraw(mainWindow);

  // Get the user camera and scene
  gazebo::rendering::UserCameraPtr cam = gazebo::gui::get_active_camera();
  QVERIFY(cam != nullptr);
  gazebo::rendering::ScenePtr scene = cam->GetScene();
  QVERIFY(scene != nullptr);

  // Get GLWidget
  gazebo::gui::GLWidget *glWidget =
    mainWindow->findChild<gazebo::gui::GLWidget *>("GLWidget");
  QVERIFY(glWidget != nullptr);

  // Get the model
  gazebo::rendering::VisualPtr modelVis = scene->GetVisual("multilink");
  QVERIFY(modelVis != nullptr);

  // Get the box link
  gazebo::rendering::VisualPtr boxLinkVis =
      scene->GetVisual("multilink::box_link");
  QVERIFY(boxLinkVis != nullptr);
  auto boxLinkPose = boxLinkVis->GetWorldPose().Ign();
  QVERIFY(boxLinkPose == boxLinkVis->GetWorldPose().Ign());

  // Get the sphere link
  gazebo::rendering::VisualPtr sphereLinkVis =
      scene->GetVisual("multilink::sphere_link");
  QVERIFY(sphereLinkVis != nullptr);
  auto sphereLinkPose = sphereLinkVis->GetWorldPose().Ign();
  QVERIFY(sphereLinkPose == sphereLinkVis->GetWorldPose().Ign());

  // Move the mouse to the corner of the screen so the dialogs open there
  QTest::mouseMove(glWidget, QPoint(-glWidget->width()*0.5,
                                    -glWidget->height()*0.5));

  this->ProcessEventsAndDraw(mainWindow);

  // Initialize dialog for the box link
  gazebo::gui::ApplyWrenchDialog *applyWrenchDialogBox =
      new gazebo::gui::ApplyWrenchDialog();
  applyWrenchDialogBox->Init("multilink", "multilink::box_link");

  this->ProcessEventsAndDraw(mainWindow);

  // Check that box dialog has focus
  QVERIFY(applyWrenchDialogBox->isActiveWindow());

  // Check mode
  QVERIFY(applyWrenchDialogBox->GetMode() ==
      gazebo::gui::ApplyWrenchDialog::Mode::NONE);

  // Get the ApplyWrenchVisual for the box
  gazebo::rendering::ApplyWrenchVisualPtr boxApplyWrenchVis =
      std::dynamic_pointer_cast<gazebo::rendering::ApplyWrenchVisual>(
      scene->GetVisual("multilink__APPLY_WRENCH__"));
  QVERIFY(boxApplyWrenchVis != nullptr);
  QVERIFY(boxApplyWrenchVis->GetParent() == boxLinkVis);

  // Check that the force visual is visible and inactive
  QVERIFY(boxApplyWrenchVis->GetForceVisual() != nullptr);
  QVERIFY(boxApplyWrenchVis->GetForceVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);

  // Check that the torque visual is visible and inactive
  QVERIFY(boxApplyWrenchVis->GetTorqueVisual() != nullptr);
  QVERIFY(boxApplyWrenchVis->GetTorqueVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);

  // Check that rot tool is not visible, not selected and in rotate mode
  QVERIFY(!boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Y));
  QVERIFY(!boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Z));
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetState() ==
      gazebo::rendering::SelectionObj::SelectionMode::SELECTION_NONE);
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetMode() ==
      gazebo::rendering::SelectionObj::SelectionMode::ROT);

  // Initialize dialog for the sphere link
  gazebo::gui::ApplyWrenchDialog *applyWrenchDialogSphere =
      new gazebo::gui::ApplyWrenchDialog();
  applyWrenchDialogSphere->Init("multilink", "multilink::sphere_link");

  this->ProcessEventsAndDraw(mainWindow);

  // Check that sphere dialog has focus
  QVERIFY(!applyWrenchDialogBox->isActiveWindow());
  QVERIFY(applyWrenchDialogSphere->isActiveWindow());

  // Check mode
  QVERIFY(applyWrenchDialogSphere->GetMode() ==
      gazebo::gui::ApplyWrenchDialog::Mode::NONE);

  // Get the ApplyWrenchVisual for the sphere
  gazebo::rendering::ApplyWrenchVisualPtr sphereApplyWrenchVis =
      std::dynamic_pointer_cast<gazebo::rendering::ApplyWrenchVisual>(
      scene->GetVisual("multilink__APPLY_WRENCH__0"));
  QVERIFY(sphereApplyWrenchVis != nullptr);
  QVERIFY(sphereApplyWrenchVis->GetParent() == sphereLinkVis);

  // Check that the force visual is visible and inactive
  QVERIFY(sphereApplyWrenchVis->GetForceVisual() != nullptr);
  QVERIFY(sphereApplyWrenchVis->GetForceVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);

  // Check that the torque visual is visible and inactive
  QVERIFY(sphereApplyWrenchVis->GetTorqueVisual() != nullptr);
  QVERIFY(sphereApplyWrenchVis->GetTorqueVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);

  // Check that rot tool is not visible, not selected and in rotate mode
  QVERIFY(!sphereApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Y));
  QVERIFY(!sphereApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Z));
  QVERIFY(sphereApplyWrenchVis->GetRotTool()->GetState() ==
      gazebo::rendering::SelectionObj::SelectionMode::SELECTION_NONE);
  QVERIFY(sphereApplyWrenchVis->GetRotTool()->GetMode() ==
      gazebo::rendering::SelectionObj::SelectionMode::ROT);

  // Give focus to main window
  mainWindow->show();
  mainWindow->raise();
  mainWindow->activateWindow();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that main window has focus
  QVERIFY(!applyWrenchDialogSphere->isActiveWindow());
  QVERIFY(mainWindow->isActiveWindow());
  QVERIFY(!applyWrenchDialogBox->isActiveWindow());

  // Get spins
  QList<QDoubleSpinBox *> spins =
      applyWrenchDialogBox->findChildren<QDoubleSpinBox *>();
  QVERIFY(spins.size() == 11u);

  // Check spins values
  for (auto i = 0; i < spins.size(); ++i)
  {
    QCOMPARE(spins[i]->value(), 0.0);
  }

  // Find the box's torque visual
  bool found = false;
  ignition::math::Vector2i
      boxForceMousePoint(glWidget->width()/2 + 10, 0);
  while (boxForceMousePoint.Y() < glWidget->height())
  {
    gazebo::rendering::VisualPtr vis = cam->GetVisual(boxForceMousePoint);
    if (vis && vis == boxApplyWrenchVis->GetForceVisual())
    {
      found = true;
      break;
    }
    boxForceMousePoint.Y() += 5;
  }

  if (!found)
  {
    QFAIL("Couldn't find force visual, interrupting test.");
    return;
  }

  // Click on the box's force visual
  QPoint boxForceClickPoint(boxForceMousePoint.X(), boxForceMousePoint.Y());
  QTest::mouseMove(glWidget, boxForceClickPoint);
  QTest::mouseClick(glWidget, Qt::LeftButton, Qt::NoModifier,
      boxForceClickPoint, 100);
  QCoreApplication::processEvents();

  // Process some events and draw the screen
  for (size_t i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check that box dialog has focus
  QVERIFY(applyWrenchDialogBox->isActiveWindow());
  QVERIFY(!applyWrenchDialogSphere->isActiveWindow());

  // Check mode
  QVERIFY(applyWrenchDialogBox->GetMode() ==
      gazebo::gui::ApplyWrenchDialog::Mode::FORCE);

  // Check that force spins changed to UnitX
  QCOMPARE(spins[0]->value(), 1.0);
  QCOMPARE(spins[1]->value(), 0.0);
  QCOMPARE(spins[2]->value(), 0.0);
  QCOMPARE(spins[3]->value(), 1.0);

  // Check that torque visual is the only one highlighted
  QVERIFY(boxApplyWrenchVis->GetForceVisual()->GetMaterialName().find(
      "Gazebo/OrangeTransparentOverlay") != std::string::npos);
  QVERIFY(boxApplyWrenchVis->GetTorqueVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);

  // Check that rot tool is visible and not highlighted
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Y));
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Z));
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetState() ==
      gazebo::rendering::SelectionObj::SelectionMode::SELECTION_NONE);

  // Find the box's torque visual
  found = false;
  ignition::math::Vector2i mousePoint(glWidget->width()/2,
                                    glWidget->height()/2 - 10);
  while (mousePoint.X() < glWidget->width())
  {
    gazebo::rendering::VisualPtr vis = cam->GetVisual(mousePoint);
    if (vis && vis == boxApplyWrenchVis->GetTorqueVisual())
    {
      found = true;
      break;
    }
    mousePoint.X() += 5;
    QTest::mouseMove(glWidget, QPoint(mousePoint.X(), mousePoint.Y()));
    this->ProcessEventsAndDraw(mainWindow, 1, 15);
  }

  if (!found)
  {
    QFAIL("Couldn't find torque visual, interrupting test.");
    return;
  }

  // Click on the box's torque visual
  QPoint clickPoint(mousePoint.X(), mousePoint.Y());
  QTest::mouseClick(glWidget, Qt::LeftButton, Qt::NoModifier, clickPoint,
      100);
  QCoreApplication::processEvents();

  this->ProcessEventsAndDraw(mainWindow);

  // Check that box dialog has focus
  QVERIFY(applyWrenchDialogBox->isActiveWindow());
  QVERIFY(!applyWrenchDialogSphere->isActiveWindow());

  // Check mode
  QVERIFY(applyWrenchDialogBox->GetMode() ==
      gazebo::gui::ApplyWrenchDialog::Mode::TORQUE);

  // Check that torque spins changed to UnitX
  QCOMPARE(spins[7]->value(), 1.0);
  QCOMPARE(spins[8]->value(), 0.0);
  QCOMPARE(spins[9]->value(), 0.0);
  QCOMPARE(spins[10]->value(), 1.0);

  // Check that torque visual is the only one highlighted
  QVERIFY(boxApplyWrenchVis->GetForceVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);
  QVERIFY(boxApplyWrenchVis->GetTorqueVisual()->GetMaterialName().find(
      "Gazebo/OrangeTransparentOverlay") != std::string::npos);

  // Check that rot tool is visible and not highlighted
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Y));
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Z));
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetState() ==
      gazebo::rendering::SelectionObj::SelectionMode::SELECTION_NONE);

  // Find the rot tool
  std::string manipState = "";
  while (mousePoint.Y() < glWidget->height())
  {
    gazebo::rendering::VisualPtr vis = cam->GetVisual(mousePoint, manipState);
    if (!vis && manipState == "rot_y")
    {
      mousePoint.Y() += 20;
      break;
    }
    mousePoint.Y() += 5;
  }

  if (manipState.empty())
  {
    return;
  }

  // Move mouse to the rot tool
  QPoint pressPoint(mousePoint.X(), mousePoint.Y());
  QTest::mouseMove(glWidget, pressPoint);

  this->ProcessEventsAndDraw(mainWindow);

  // Check that rot tool is highlighted
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetState() ==
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Y);

  // Get/check the initial visual poses
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetPose().Ign() ==
      ignition::math::Pose3d::Zero);

  auto boxTorquePose0 = boxApplyWrenchVis->GetTorqueVisual()->GetPose().Ign();
  auto boxForcePose0 = boxApplyWrenchVis->GetForceVisual()->GetPose().Ign();

  // Drag the tool
  QTestEventList events;
  events.addMousePress(Qt::LeftButton, Qt::NoModifier, pressPoint, 100);
  for (size_t i = 0; i < 10; ++i)
  {
    events.addMouseMove(QPoint(pressPoint.x(), pressPoint.y()-10*i), 100);
  }
  events.simulate(glWidget);

  this->ProcessEventsAndDraw(mainWindow);

  // Check mode
  QVERIFY(applyWrenchDialogBox->GetMode() ==
      gazebo::gui::ApplyWrenchDialog::Mode::TORQUE);

  // Check that only rot tool and torque were rotated
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetPose().Ign().Pos() ==
      ignition::math::Vector3d::Zero);
  QVERIFY(boxApplyWrenchVis->GetRotTool()->GetPose().Ign().Rot() !=
      ignition::math::Vector3d::Zero);
  QVERIFY(boxApplyWrenchVis->GetTorqueVisual()->GetPose().Ign() !=
      boxTorquePose0);
  QVERIFY(boxApplyWrenchVis->GetForceVisual()->GetPose().Ign() ==
      boxForcePose0);

  // Check that only torque spins X and Z changed
  QVERIFY(spins[7]->value() < 1.0);
  QCOMPARE(spins[8]->value(), 0.0);
  QVERIFY(spins[9]->value() > 0.0);
  QCOMPARE(spins[10]->value(), 1.0);

  // Trigger a manipulation mode
  gazebo::gui::g_translateAct->trigger();

  // Check that the force visual is visible and inactive
  QVERIFY(boxApplyWrenchVis->GetForceVisual() != nullptr);
  QVERIFY(boxApplyWrenchVis->GetForceVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);

  // Check that the torque visual is visible and inactive
  QVERIFY(boxApplyWrenchVis->GetTorqueVisual() != nullptr);
  QVERIFY(boxApplyWrenchVis->GetTorqueVisual()->GetMaterialName().find(
      "Gazebo/DarkOrangeTransparentOverlay") != std::string::npos);

  // Check that rot tool is not visible
  QVERIFY(!boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Y));
  QVERIFY(!boxApplyWrenchVis->GetRotTool()->GetHandleVisible(
      gazebo::rendering::SelectionObj::SelectionMode::ROT_Z));

  // Clean up
  delete applyWrenchDialogBox;
  delete applyWrenchDialogSphere;

  cam->Fini();
  mainWindow->close();
  delete mainWindow;
}

// Generate a main function for the test
QTEST_MAIN(ApplyWrenchDialog_TEST)
