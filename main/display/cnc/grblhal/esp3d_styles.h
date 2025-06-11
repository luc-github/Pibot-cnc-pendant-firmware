/*
  esp3d_styles.h - ESP3D screens styles definition

  Copyright (c) 2022 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <stdio.h>

#include "esp3d_styles_res.h"
#include "lvgl.h"

// Colors definition

// Screen colors
#define ESP3D_SCREEN_BACKGROUND_COLOR      0x000000
#define ESP3D_SCREEN_BACKGROUND_TEXT_COLOR 0xFFFFFF

// Circular menu constants
#define ESP3D_MAIN_MENU_DIAMETER              240  // pixels
#define ESP3D_MAIN_MENU_BORDER_WIDTH          3    // pixels
#define ESP3D_MAIN_MENU_MAX_SECTIONS          14   // Maximum number of sections
#define ESP3D_MAIN_MENU_INNER_DIAMETER 130  // Diamètre du cercle central
#define ESP3D_MAIN_MENU_INNER_ARC_WIDTH       54   // Largeur de l'arc intérieur
#define ESP3D_MAIN_MENU_ICON_ZONE_DIAMETER    40   // Diamètre des zones cliquables

// Buttom Buttons
#define ESP3D_BOTTOM_BUTTON_SIZE      50   // Taille des boutons du bas
#define ESP3D_BOTTOM_BUTTON_SPACING   25   // Espacement entre boutons du bas
#define ESP3D_LONG_PRESS_THRESHOLD_MS 200  // Seuil pour un appui long (ms)

// Main menu colors definition
#define ESP3D_MENU_BORDER_COLOR   0x404040  // Gris foncé pour la bordure
#define ESP3D_MENU_INNER_COLOR    0x222222  // Gris clair pour le cercle central
#define ESP3D_MENU_SELECTOR_COLOR 0x00BFFF  // Bleu clair pour la sélection
#define ESP3D_MENU_ICON_COLOR     0xFFFFFF  // Blanc pour les icônes
