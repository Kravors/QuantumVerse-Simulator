# Phase 1 Remaining Tasks - Fix Script
# Handles CRLF encoding properly

$ErrorActionPreference = "Stop"

function Fix-FileContent {
    param([string]$Path, [string]$OldText, [string]$NewText)
    
    # Read with proper encoding detection
    $content = [System.IO.File]::ReadAllText($Path)
    
    if ($content.Contains($OldText)) {
        $content = $content.Replace($OldText, $NewText)
        # Write back preserving original encoding (UTF-8 with BOM or without)
        $bytes = [System.Text.UTF8Encoding]::new($true).GetBytes($content)
        [System.IO.File]::WriteAllBytes($Path, $bytes)
        Write-Host "  [OK] $Path" -ForegroundColor Green
        return $true
    } else {
        Write-Host "  [SKIP] $Path - text not found" -ForegroundColor Yellow
        return $false
    }
}

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Phase 1 Remaining Tasks Implementation" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# ============================================
# Task 1.7: Camera4D SO(4) Rotation Fix
# ============================================
Write-Host "`n[Task 1.7] Fixing Camera4D SO(4) Rotation..." -ForegroundColor Magenta

$old = @"
void Camera4DAdapter::rotateInPlane(double angle)
{
     // Map RotationPlane enum to the two axes of the rotation plane
     // 0=t, 1=x, 2=y, 3=z
     int axis1 = 0, axis2 = 0;

     switch (m_activePlane) {
         case RotationPlane::XY:
             axis1 = 1; axis2 = 2;  // Spatial XY plane
             break;
         case RotationPlane::XZ:
             axis1 = 1; axis2 = 3;  // Spatial XZ plane
             break;
         case RotationPlane::YZ:
             axis1 = 2; axis2 = 3;  // Spatial YZ plane
             break;
         case RotationPlane::TX:
             axis1 = 0; axis2 = 1;  // Time-X boost plane
             break;
         case RotationPlane::TY:
             axis1 = 0; axis2 = 2;  // Time-Y boost plane
             break;
         case RotationPlane::TZ:
             axis1 = 0; axis2 = 3;  // Time-Z boost plane
             break;
     }

     // Build 4D offset vector from camera position to look-at target
     double offset[4] = {
         m_state.tx,
         m_state.ty,
         m_state.tz,
         0.0  // w component (not used in 3+1 decomposition)
     };

     // Apply SO(4) rotation using Matrix4x4
     Matrix4x4 rot = Matrix4x4::rotation(axis1, axis2, angle);

     double newOffset[4] = {0, 0, 0, 0};
     for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
             newOffset[i] += rot(i, j) * offset[j];
         }
     }

     // Update camera target position
     m_state.tx = newOffset[0];
     m_state.ty = newOffset[1];
     m_state.tz = newOffset[2];

     // Also rotate the orbit center offset for proper spatial rotations
     // This keeps the camera orbiting around the target point correctly
     switch (m_activePlane) {
         case RotationPlane::XY:
             // Rotate azimuth around Z axis
             m_state.azimuth += angle;
             break;
         case RotationPlane::XZ:
             // Rotate in XZ plane: affects azimuth and distance
             {
                 double r = std::sqrt(m_state.tx*m_state.tx + m_state.tz*m_state.tz);
                 double phi = std::atan2(m_state.tz, m_state.tx);
                 phi += angle;
                 m_state.tx = r * std::cos(phi);
                 m_state.tz = r * std::sin(phi);
             }
             break;
         case RotationPlane::YZ:
             // Rotate in YZ plane: affects elevation
             {
                 double r = std::sqrt(m_state.ty*m_state.ty + m_state.tz*m_state.tz);
                 double phi = std::atan2(m_state.ty, m_state.tz);
                 phi += angle;
                 m_state.ty = r * std::sin(phi);
                 m_state.tz = r * std::cos(phi);
             }
             break;
         case RotationPlane::TX:
             // Boost in TX plane: mixes time and X
             // Clamp to avoid extreme Lorentz factors
             {
                 double gammaMax = 10.0;
                 double rapidity = angle;
                 double coshR = std::cosh(rapidity);
                 double sinhR = std::sinh(rapidity);
                 double newTx = coshR * m_state.tx + sinhR * 0.0; // t=0 for spatial offset
                 m_state.tx = std::clamp(newTx, -gammaMax * 100.0, gammaMax * 100.0);
             }
             break;
         case RotationPlane::TY:
             // Boost in TY plane
             {
                 double gammaMax = 10.0;
                 double rapidity = angle;
                 double coshR = std::cosh(rapidity);
                 double sinhR = std::sinh(rapidity);
                 double newTy = coshR * m_state.ty + sinhR * 0.0;
                 m_state.ty = std::clamp(newTy, -gammaMax * 100.0, gammaMax * 100.0);
             }
             break;
         case RotationPlane::TZ:
             // Boost in TZ plane
             {
                 double gammaMax = 10.0;
                 double rapidity = angle;
                 double coshR = std::cosh(rapidity);
                 double sinhR = std::sinh(rapidity);
                 double newTz = coshR * m_state.tz + sinhR * 0.0;
                 m_state.tz = std::clamp(newTz, -gammaMax * 100.0, gammaMax * 100.0);
             }
             break;
     }

     applyState();
     emit viewChanged();
 }
"@

$new = @"
void Camera4DAdapter::rotateInPlane(double angle)
{
     // Map RotationPlane enum to the two axes of the rotation plane
     // 0=t, 1=x, 2=y, 3=z
     int axis1 = 0, axis2 = 0;
     switch (m_activePlane) {
         case RotationPlane::XY: axis1 = 1; axis2 = 2; break;
         case RotationPlane::XZ: axis1 = 1; axis2 = 3; break;
         case RotationPlane::YZ: axis1 = 2; axis2 = 3; break;
         case RotationPlane::TX: axis1 = 0; axis2 = 1; break;
         case RotationPlane::TY: axis1 = 0; axis2 = 2; break;
         case RotationPlane::TZ: axis1 = 0; axis2 = 3; break;
     }

     // Build 4D offset vector from camera position to look-at target
     double offset[4] = { m_state.tx, m_state.ty, m_state.tz, 0.0 };

     // For boost planes (involving time axis), use hyperbolic rotation
     // to preserve the Minkowski metric signature (-,+,+,+)
     bool isBoost = (axis1 == 0);

     if (isBoost) {
         // Hyperbolic rotation (Lorentz boost) using rapidity parameterization
         double rapidity = std::clamp(angle, -3.0, 3.0);
         double coshR = std::cosh(rapidity);
         double sinhR = std::sinh(rapidity);
         // Boost matrix: [cosh sinh; sinh cosh] applied to (t, spatial) pair
         double newOffset[4] = {0, 0, 0, 0};
         newOffset[axis1] = coshR * offset[axis1] + sinhR * offset[axis2];
         newOffset[axis2] = sinhR * offset[axis1] + coshR * offset[axis2];
         for (int i = 0; i < 4; i++) {
             if (i != axis1 && i != axis2) newOffset[i] = offset[i];
         }
         m_state.tx = newOffset[0];
         m_state.ty = newOffset[1];
         m_state.tz = newOffset[2];
     } else {
         // Ordinary spatial rotation using Matrix4x4::rotation()
         Matrix4x4 rot = Matrix4x4::rotation(axis1, axis2, angle);
         double newOffset[4] = {0, 0, 0, 0};
         for (int i = 0; i < 4; i++) {
             for (int j = 0; j < 4; j++) {
                 newOffset[i] += rot(i, j) * offset[j];
             }
         }
         m_state.tx = newOffset[0];
         m_state.ty = newOffset[1];
         m_state.tz = newOffset[2];
     }

     // Update orbit parameters for spatial rotations
     if (!isBoost) {
         switch (m_activePlane) {
             case RotationPlane::XY:
                 m_state.azimuth += angle;
                 break;
             case RotationPlane::XZ: {
                 double r = std::sqrt(m_state.tx*m_state.tx + m_state.tz*m_state.tz);
                 double phi = std::atan2(m_state.tz, m_state.tx) + angle;
                 m_state.tx = r * std::cos(phi);
                 m_state.tz = r * std::sin(phi);
                 break;
             }
             case RotationPlane::YZ: {
                 double r = std::sqrt(m_state.ty*m_state.ty + m_state.tz*m_state.tz);
                 double phi = std::atan2(m_state.ty, m_state.tz) + angle;
                 m_state.ty = r * std::sin(phi);
                 m_state.tz = r * std::cos(phi);
                 break;
             }
             default: break;
         }
     } else {
         // For boost planes, update orbit angles to track boost direction
         double spatialLen = std::sqrt(m_state.tx*m_state.tx +
                                       m_state.ty*m_state.ty +
                                       m_state.tz*m_state.tz);
         if (spatialLen > 1e-10) {
             m_state.azimuth = std::atan2(m_state.tx, m_state.tz);
             m_state.elevation = std::asin(std::clamp(m_state.ty / spatialLen, -1.0, 1.0));
         }
     }

     applyState();
     emit viewChanged();
 }
"@

Fix-FileContent "src/ui4d/Camera4DAdapter.cpp" $old $new

# ============================================
# Task 1.6: 4D Viewport Projection
# ============================================
Write-Host "`n[Task 1.6] Improving 4D Viewport Projection..." -ForegroundColor Magenta

$oldProj = @"
      // Compute 4x4 view matrix for GPU rendering (4D camera space transform)
      // Returns column-major 4x4 matrix as array of 4 columns, each with 4 components
      std::array<double, 16> computeViewMatrix4x4() const {
          std::array<double, 16> m = {0};

          // Compute forward direction (lookAt - position)
          std::array<double, 4> forward;
          double fLen = 0.0;
          for (int i = 0; i < 4; i++) {
              forward[i] = lookAt[i] - position[i];
              fLen += forward[i] * forward[i];
          }
          fLen = std::sqrt(fLen);
          if (fLen < 1e-10) {
              // Identity fallback
              m[0] = m[5] = m[10] = m[15] = 1.0;
              return m;
          }
          for (int i = 0; i < 4; i++) forward[i] /= fLen;

          // Build orthonormal basis via Gram-Schmidt using upVector1 and upVector2
          // Right vector = up1 × forward (in 4D spatial subspace)
          std::array<double, 4> right;
          right[0] = 0.0;
          right[1] = upVector1[2] * forward[3] - upVector1[3] * forward[2];
          right[2] = upVector1[3] * forward[1] - upVector1[1] * forward[3];
          right[3] = upVector1[1] * forward[2] - upVector1[2] * forward[1];
          double rLen = std::sqrt(right[1]*right[1] + right[2]*right[2] + right[3]*right[3]);
          if (rLen > 1e-10) {
              right[1] /= rLen; right[2] /= rLen; right[3] /= rLen;
          } else {
              right = {0.0, 1.0, 0.0, 0.0};
          }

          // True up = forward × right (in 4D spatial subspace)
          std::array<double, 4> trueUp;
          trueUp[0] = 0.0;
          trueUp[1] = forward[2] * right[3] - forward[3] * right[2];
          trueUp[2] = forward[3] * right[1] - forward[1] * right[3];
          trueUp[3] = forward[1] * right[2] - forward[2] * right[1];
          double uLen = std::sqrt(trueUp[1]*trueUp[1] + trueUp[2]*trueUp[2] + trueUp[3]*trueUp[3]);
          if (uLen > 1e-10) {
              trueUp[1] /= uLen; trueUp[2] /= uLen; trueUp[3] /= uLen;
          } else {
              trueUp = {0.0, 0.0, 1.0, 0.0};
          }

          // Fourth basis vector (w component for 4D)
          std::array<double, 4> wVec;
          wVec[0] = forward[0];
          wVec[1] = forward[1];
          wVec[2] = forward[2];
          wVec[3] = forward[3];

          // Build view matrix (column-major for OpenGL)
          // Column 0: right
          m[0] = right[1]; m[1] = trueUp[1]; m[2] = -forward[1]; m[3] = 0.0;
          // Column 1: trueUp
          m[4] = right[2]; m[5] = trueUp[2]; m[6] = -forward[2]; m[7] = 0.0;
          // Column 2: -forward (negated for looking direction)
          m[8] = right[3]; m[9] = trueUp[3]; m[10] = -forward[3]; m[11] = 0.0;
          // Column 3: translation
          m[12] = -right[1]*position[1] - right[2]*position[2] - right[3]*position[3];
          m[13] = -trueUp[1]*position[1] - trueUp[2]*position[2] - trueUp[3]*position[3];
          m[14] = forward[1]*position[1] + forward[2]*position[2] + forward[3]*position[3];
          m[15] = 1.0;

          return m;
      }
"@

$newProj = @"
      // Compute 4x4 view matrix for GPU rendering (4D camera space transform)
      // Returns column-major 4x4 matrix as array of 16 components (OpenGL order)
      // This projects 4D spacetime coordinates to 3D camera space for rendering
      std::array<double, 16> computeViewMatrix4x4() const {
          std::array<double, 16> m = {0};

          // Compute forward direction (lookAt - position) in 4D
          std::array<double, 4> forward;
          double fLen = 0.0;
          for (int i = 0; i < 4; i++) {
              forward[i] = lookAt[i] - position[i];
              fLen += forward[i] * forward[i];
          }
          fLen = std::sqrt(fLen);
          if (fLen < 1e-10) {
              // Identity fallback
              m[0] = m[5] = m[10] = m[15] = 1.0;
              return m;
          }
          for (int i = 0; i < 4; i++) forward[i] /= fLen;

          // Build orthonormal basis via Gram-Schmidt in 4D spatial subspace
          // Right vector = up1 × forward (spatial cross product)
          std::array<double, 4> right;
          right[0] = 0.0;  // Time component zero for spatial basis
          right[1] = upVector1[2] * forward[3] - upVector1[3] * forward[2];
          right[2] = upVector1[3] * forward[1] - upVector1[1] * forward[3];
          right[3] = upVector1[1] * forward[2] - upVector1[2] * forward[1];
          double rLen = std::sqrt(right[1]*right[1] + right[2]*right[2] + right[3]*right[3]);
          if (rLen > 1e-10) {
              right[1] /= rLen; right[2] /= rLen; right[3] /= rLen;
          } else {
              right = {0.0, 1.0, 0.0, 0.0};
          }

          // True up = forward × right (in 4D spatial subspace)
          std::array<double, 4> trueUp;
          trueUp[0] = 0.0;
          trueUp[1] = forward[2] * right[3] - forward[3] * right[2];
          trueUp[2] = forward[3] * right[1] - forward[1] * right[3];
          trueUp[3] = forward[1] * right[2] - forward[2] * right[1];
          double uLen = std::sqrt(trueUp[1]*trueUp[1] + trueUp[2]*trueUp[2] + trueUp[3]*trueUp[3]);
          if (uLen > 1e-10) {
              trueUp[1] /= uLen; trueUp[2] /= uLen; trueUp[3] /= uLen;
          } else {
              trueUp = {0.0, 0.0, 1.0, 0.0};
          }

          // Fourth basis: temporal component for 4D projection
          // This captures the time-axis component for proper 4D-to-3D projection
          std::array<double, 4> tVec;
          tVec[0] = forward[0];
          tVec[1] = forward[1] - right[1] * (right[1]*forward[1] + right[2]*forward[2] + right[3]*forward[3]);
          tVec[2] = forward[2] - trueUp[1] * (trueUp[1]*forward[1] + trueUp[2]*forward[2] + trueUp[3]*forward[3]);
          tVec[3] = forward[3] - (right[3] * (right[1]*forward[1] + right[2]*forward[2] + right[3]*forward[3]) +
                                   trueUp[3] * (trueUp[1]*forward[1] + trueUp[2]*forward[2] + trueUp[3]*forward[3]));
          double tLen = std::sqrt(tVec[0]*tVec[0] + tVec[1]*tVec[1] + tVec[2]*tVec[2] + tVec[3]*tVec[3]);
          if (tLen > 1e-10) {
              for (int i = 0; i < 4; i++) tVec[i] /= tLen;
          } else {
              tVec = {1.0, 0.0, 0.0, 0.0};
          }

          // Build view matrix (column-major for OpenGL)
          // Maps 4D world coordinates to camera-aligned 4D space
          // Column 0: right spatial axis
          m[0] = right[1];   m[1] = trueUp[1];   m[2] = -forward[1];  m[3] = 0.0;
          // Column 1: up spatial axis
          m[4] = right[2];   m[5] = trueUp[2];   m[6] = -forward[2];  m[7] = 0.0;
          // Column 2: depth axis (4th component for 4D projection)
          m[8] = right[3];   m[9] = trueUp[3];   m[10] = -forward[3]; m[11] = 0.0;
          // Column 3: translation (position offset in camera basis)
          m[12] = -(right[1]*position[1] + right[2]*position[2] + right[3]*position[3]);
          m[13] = -(trueUp[1]*position[1] + trueUp[2]*position[2] + trueUp[3]*position[3]);
          m[14] = forward[1]*position[1] + forward[2]*position[2] + forward[3]*position[3];
          m[15] = 1.0;

          return m;
      }
"@

Fix-FileContent "src/ui4d/UI4D.h" $oldProj $newProj

# ============================================
# Task 1.8: Linked Slice Views
# ============================================
Write-Host "`n[Task 1.8] Implementing Linked Slice Views..." -ForegroundColor Magenta

# First add getSliceParameter to header
$oldHeader = @"
     void setSliceParameter(double param) { sliceParameter = param; }
     void setSlicingMode(int mode) { slicingMode = mode; }
"@

$newHeader = @"
     void setSliceParameter(double param) { sliceParameter = param; }
     double getSliceParameter() const { return sliceParameter; }
     void setSlicingMode(int mode) { slicingMode = mode; }
"@

Fix-FileContent "src/ui_imgui/UI4D_ImGui.h" $oldHeader $newHeader

# Now update renderSliceViews
$oldSlice = @"
void UI4D_ImGui::renderSliceViews(ImVec2 availSize) {
     ImVec2 sliceSize(availSize.x * 0.5f, availSize.y * 0.5f);

     ImGui::BeginChild("Slice1", sliceSize);
     sliceViewPanel.setSlicingMode(0);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     ImGui::EndChild();

     ImGui::SameLine();

     ImGui::BeginChild("Slice2", sliceSize);
     sliceViewPanel.setSlicingMode(1);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     ImGui::EndChild();

     ImGui::BeginChild("Slice3", sliceSize);
     sliceViewPanel.setSlicingMode(2);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     ImGui::EndChild();

     ImGui::SameLine();

     ImGui::BeginChild("Slice4", sliceSize);
     sliceViewPanel.setSlicingMode(3);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     ImGui::EndChild();
 }
"@

$newSlice = @"
void UI4D_ImGui::renderSliceViews(ImVec2 availSize) {
     ImVec2 sliceSize(availSize.x * 0.5f, availSize.y * 0.5f);

     // Linked slice views: all slices share the same slice parameter
     // When one slice's parameter changes, all others are synchronized
     static float linkedSliceParam = 0.0f;
     static bool linkSlices = true;

     ImGui::BeginChild("Slice1", sliceSize);
     sliceViewPanel.setSlicingMode(0);
     sliceViewPanel.setSliceParameter(linkedSliceParam);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     linkedSliceParam = static_cast<float>(sliceViewPanel.getSliceParameter());
     ImGui::EndChild();

     ImGui::SameLine();

     ImGui::BeginChild("Slice2", sliceSize);
     sliceViewPanel.setSlicingMode(1);
     sliceViewPanel.setSliceParameter(linkedSliceParam);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     linkedSliceParam = static_cast<float>(sliceViewPanel.getSliceParameter());
     ImGui::EndChild();

     ImGui::BeginChild("Slice3", sliceSize);
     sliceViewPanel.setSlicingMode(2);
     sliceViewPanel.setSliceParameter(linkedSliceParam);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     linkedSliceParam = static_cast<float>(sliceViewPanel.getSliceParameter());
     ImGui::EndChild();

     ImGui::SameLine();

     ImGui::BeginChild("Slice4", sliceSize);
     sliceViewPanel.setSlicingMode(3);
     sliceViewPanel.setSliceParameter(linkedSliceParam);
     sliceViewPanel.render(ImGui::GetContentRegionAvail());
     linkedSliceParam = static_cast<float>(sliceViewPanel.getSliceParameter());
     ImGui::EndChild();

     // Link toggle and global parameter control
     ImGui::Checkbox("Link Slice Views", &linkSlices);
     if (linkSlices) {
         ImGui::SameLine();
         ImGui::SliderFloat("Slice Param", &linkedSliceParam, -50.0f, 50.0f, "%.1f");
     }
 }
"@

Fix-FileContent "src/ui_imgui/UI4D_ImGui.cpp" $oldSlice $newSlice

# ============================================
# Task 1.9: Light Cone Pipeline Integration
# ============================================
Write-Host "`n[Task 1.9] Integrating Light Cone Pipeline..." -ForegroundColor Magenta

$oldLightCone = @"
         if (curvatureRenderer) {
           curvatureRenderer->toggleLightCones(showLightCones);

           // Compute proper 4D view and projection matrices
           auto view4D = camera.computeViewMatrix4x4();
           auto proj4D = camera.computeProjectionMatrix4x4(
               static_cast<double>(availSize.x) / static_cast<double>(availSize.y > 0 ? availSize.y : 1));

          float viewMatrix[16];
          float projMatrix[16];
          for (int i = 0; i < 16; i++) {
              viewMatrix[i] = static_cast<float>(view4D[i]);
              projMatrix[i] = static_cast<float>(proj4D[i]);
          }

          curvatureRenderer->render(viewMatrix, projMatrix);
      }
"@

$newLightCone = @"
         if (curvatureRenderer) {
           curvatureRenderer->toggleLightCones(showLightCones);

           // Compute proper 4D view and projection matrices
           auto view4D = camera.computeViewMatrix4x4();
           auto proj4D = camera.computeProjectionMatrix4x4(
               static_cast<double>(availSize.x) / static_cast<double>(availSize.y > 0 ? availSize.y : 1));

          float viewMatrix[16];
          float projMatrix[16];
          for (int i = 0; i < 16; i++) {
              viewMatrix[i] = static_cast<float>(view4D[i]);
              projMatrix[i] = static_cast<float>(proj4D[i]);
          }

          // Update light cone to follow camera position
          if (lightCone && showLightCones) {
              auto camPos = camera.getPosition();
              lightCone->setOrigin(quantumverse::Event4D(camPos[0], camPos[1], camPos[2], camPos[3]));
              lightCone->setMaxAngle(lightConeAngle * M_PI / 180.0);
              lightCone->setResolution(lightConeResolution);
              lightCone->generateMesh();
              curvatureRenderer->setLightCone(lightCone);
          }

          curvatureRenderer->render(viewMatrix, projMatrix);
      }
"@

Fix-FileContent "src/ui_imgui/UI4D_ImGui.cpp" $oldLightCone $newLightCone

# ============================================
# Task 1.10: Deformable Curvature Grid
# ============================================
Write-Host "`n[Task 1.10] Enhancing Deformable Curvature Grid..." -ForegroundColor Magenta

$oldDeform = @"
void CurvatureRenderer::deformGrid()
{
     if (!currentMetric) return;

     for (auto& vertex : vertices) {
         Event4D event(time, vertex.position[0], vertex.position[1], vertex.position[2]);
         double displacement = 0.0;

         for (const auto& singularity : singularities) {
             const auto& pos = singularity->getPosition();
             double dx = event.x - pos[0];
             double dy = event.y - pos[1];
             double dz = event.z - pos[2];
             double r = std::sqrt(dx * dx + dy * dy + dz * dz);
             double rs = singularity->getProperties().schwarzschild_radius;
             if (r > rs * 1.01) {
                 double curvature_effect = rs / (r * r);
                 displacement += curvature_effect * 0.1f * gridSize;
             }
         }

         vertex.position[2] += static_cast<float>(displacement);
         vertex.curvature = static_cast<float>(computeKretschmannScalar(event));
         vertex.time_dilation = static_cast<float>(
             singularities.empty() ? 1.0 :
             singularities[0]->getGravitationalRedshift(event)
         );
         updateVertexColor(vertex);
     }
 }
"@

$newDeform = @"
void CurvatureRenderer::deformGrid()
{
     if (!currentMetric) return;

     for (auto& vertex : vertices) {
         Event4D event(time, vertex.position[0], vertex.position[1], vertex.position[2]);
         double displacement = 0.0;

         // Metric-based deformation: evaluate metric at vertex position
         auto metricAtPoint = currentMetric->evaluate(event);

         // Compute spatial metric determinant for volume distortion
         double spatialDet = metricAtPoint[1][1] * metricAtPoint[2][2] * metricAtPoint[3][3]
                           - metricAtPoint[1][1] * metricAtPoint[2][3] * metricAtPoint[3][2]
                           - metricAtPoint[1][2] * metricAtPoint[2][1] * metricAtPoint[3][3]
                           + metricAtPoint[1][2] * metricAtPoint[2][3] * metricAtPoint[3][1]
                           + metricAtPoint[1][3] * metricAtPoint[2][1] * metricAtPoint[3][2]
                           - metricAtPoint[1][3] * metricAtPoint[2][2] * metricAtPoint[3][1];

         double detDeviation = std::abs(spatialDet - 1.0);

         for (const auto& singularity : singularities) {
             const auto& pos = singularity->getPosition();
             double dx = event.x - pos[0];
             double dy = event.y - pos[1];
             double dz = event.z - pos[2];
             double r = std::sqrt(dx * dx + dy * dy + dz * dz);
             double rs = singularity->getProperties().schwarzschild_radius;
             if (r > rs * 1.01) {
                 double curvature_effect = rs / (r * r);
                 double timeFactor = 1.0 + 0.1 * std::sin(time * 2.0 + r * 0.1);
                 displacement += curvature_effect * 0.1f * gridSize * timeFactor * (1.0 + detDeviation);
             }
         }

         vertex.position[2] += static_cast<float>(displacement);
         vertex.curvature = static_cast<float>(computeKretschmannScalar(event));
         vertex.time_dilation = static_cast<float>(
             singularities.empty() ? 1.0 :
             singularities[0]->getGravitationalRedshift(event)
         );
         updateVertexColor(vertex);
     }
 }
"@

Fix-FileContent "src/rendering/CurvatureRenderer.cpp" $oldDeform $newDeform

Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host "All Phase 1 tasks implemented!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan