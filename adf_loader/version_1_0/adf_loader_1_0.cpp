//==============================================================================
/*
    Software License Agreement (BSD License)
    Copyright (c) 2020, AMBF
    (https://github.com/WPI-AIM/ambf)

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

    * Neither the name of authors nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    \author    <amunawar@wpi.edu>
    \author    Adnan Munawar
    \version   1.0$
*/
//==============================================================================

#include "adf_loader_1_0.h"
#include <algorithm>
//#include "afUtils.h"

using namespace ambf;
using namespace adf_loader_1_0;
using namespace std;

///
/// \brief toXYZ<cVector3d>
/// \param node
/// \return
///
afVector3d ADFUtils::positionFromNode(YAML::Node* a_node){
    afVector3d v;
    YAML::Node & node = *a_node;
    v(0) = node["x"].as<double>();
    v(1) = node["y"].as<double>();
    v(2) = node["z"].as<double>();
    return v;
}

///
/// \brief toRPY<cVector3>
/// \param node
/// \return
///
afMatrix3d ADFUtils::rotationFromNode(YAML::Node *node){
    afMatrix3d v;
    double r, p, y;
    r = (*node)["r"].as<double>();
    p = (*node)["p"].as<double>();
    y = (*node)["y"].as<double>();
    v.setRPY(r, p, y);
    return v;
}


///
/// \brief ADFUtils::getColorAttribsFromNode
/// \param a_node
/// \param a_color
/// \return
///
bool ADFUtils::getColorAttribsFromNode(YAML::Node *a_node, afColorAttributes* a_color)
{
    YAML::Node& matNode = *a_node;

    YAML::Node colorNameNode = matNode["color"];
    YAML::Node colorRGBANode = matNode["color rgba"];
    YAML::Node colorComponentsNode = matNode["color components"];

    afColorAttributes& colorAttribs = *a_color;

    if(colorRGBANode.IsDefined()){
        colorAttribs.m_diffuse(0) = colorRGBANode["r"].as<double>();
        colorAttribs.m_diffuse(1) = colorRGBANode["g"].as<double>();
        colorAttribs.m_diffuse(2) = colorRGBANode["b"].as<double>();
        colorAttribs.m_alpha = colorRGBANode["a"].as<double>();
        float level = 0.5;
        colorAttribs.m_ambient = colorAttribs.m_diffuse * level;
    }
    else if(colorComponentsNode.IsDefined()){
        if (colorComponentsNode["diffuse"].IsDefined()){
            colorAttribs.m_diffuse(0) = colorComponentsNode["diffuse"]["r"].as<double>();
            colorAttribs.m_diffuse(1) = colorComponentsNode["diffuse"]["g"].as<double>();
            colorAttribs.m_diffuse(2) = colorComponentsNode["diffuse"]["b"].as<double>();
        }
        if (colorComponentsNode["specular"].IsDefined()){
            colorAttribs.m_specular(0) = colorComponentsNode["specular"]["r"].as<double>();
            colorAttribs.m_specular(1) = colorComponentsNode["specular"]["g"].as<double>();
            colorAttribs.m_specular(2) = colorComponentsNode["specular"]["b"].as<double>();
        }
        if (colorComponentsNode["emission"].IsDefined()){
            colorAttribs.m_emission(0) = colorComponentsNode["emission"]["r"].as<double>();
            colorAttribs.m_emission(1) = colorComponentsNode["emission"]["g"].as<double>();
            colorAttribs.m_emission(2) = colorComponentsNode["emission"]["b"].as<double>();
        }
        if (colorComponentsNode["ambient"].IsDefined()){
            double level =  colorComponentsNode["ambient"]["level"].as<double>();
            colorAttribs.m_ambient = colorAttribs.m_diffuse * level;
        }
        if (colorComponentsNode["shininess"].IsDefined()){
            colorAttribs.m_shininess = colorComponentsNode["shininess"].as<uint>();
        }
        colorAttribs.m_alpha = colorComponentsNode["transparency"].as<double>();
    }
//    else if(colorNameNode.IsDefined()){
//        vector<double> rgba = afConfigHandler::getColorRGBA(colorNameNode.as<string>());
//        mat.setColorf(rgba[0], rgba[1], rgba[2], rgba[3]);
//    }

    return true;
}


bool ADFUtils::getShaderAttribsFromNode(YAML::Node *a_node, afShaderAttributes *attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node shadersNode = node["shaders"];

    bool valid = true;

    if (shadersNode.IsDefined()){
        afPath localPath = shadersNode["path"].as<string>();

        attribs->m_vtxFilepath = shadersNode["vertex"].as<string>();
        attribs->m_fragFilepath = shadersNode["fragment"].as<string>();

        attribs->m_vtxFilepath = localPath / attribs->m_vtxFilepath;
        attribs->m_fragFilepath = localPath / attribs->m_fragFilepath;

        attribs->m_shaderDefined = true;
    }
    else{
        attribs->m_shaderDefined = false;
        valid = false;
    }

    return valid;
}

bool ADFUtils::getVisualAttribsFromNode(YAML::Node *a_node, afVisualAttributes *attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node meshNode = node["mesh"];
    YAML::Node shapeNode = node["shape"];
    YAML::Node compoundShapeNode = node["compound shape"];
    YAML::Node geometryNode = node["geometry"];
    YAML::Node meshPathHRNode = node["high resolution path"];
    YAML::Node colorNode = node["color"];

    bool valid = false;

    string shape_str;
    attribs->m_geometryType = afGeometryType::INVALID;

    afPath localPath;

    // Each rigid body can have a seperate path for its low and high res meshes
    // Incase they are defined, we use these paths and if they are not, we use
    // the paths for the whole file
    if (meshPathHRNode.IsDefined()){
        localPath = meshPathHRNode.as<string>();
    }

    if (shapeNode.IsDefined()){
        attribs->m_geometryType = afGeometryType::SINGLE_SHAPE;
        shape_str = shapeNode.as<string>();
        afPrimitiveShapeAttributes shapeAttribs;
        shapeAttribs.setShapeType(ADFUtils::getShapeTypeFromString(shape_str));
        ADFUtils::copyPrimitiveShapeData(&geometryNode, &shapeAttribs);
        attribs->m_primitiveShapes.push_back(shapeAttribs);
    }
    else if (compoundShapeNode.IsDefined()){
        attribs->m_geometryType = afGeometryType::COMPOUND_SHAPE;
        for(uint shapeIdx = 0 ; shapeIdx < compoundShapeNode.size() ; shapeIdx++){
            shape_str = compoundShapeNode[shapeIdx]["shape"].as<string>();
            geometryNode = compoundShapeNode[shapeIdx]["geometry"];
            YAML::Node shapeOffset = compoundShapeNode[shapeIdx]["offset"];

            afPrimitiveShapeAttributes shapeAttribs;
            shapeAttribs.setShapeType(ADFUtils::getShapeTypeFromString(shape_str));
            ADFUtils::copyPrimitiveShapeData(&geometryNode, &shapeAttribs);
            ADFUtils::copyShapeOffsetData(&shapeOffset, &shapeAttribs);
            attribs->m_primitiveShapes.push_back(shapeAttribs);
        }
    }
    else if(meshNode.IsDefined()){
        attribs->m_meshFilepath = localPath / meshNode.as<string>();
        if (!attribs->m_meshFilepath.c_str().empty()){
            attribs->m_geometryType = afGeometryType::MESH;
        }
        else{
            valid = false;
        }
    }
    else{
        valid = false;
    }

    ADFUtils utils;
    utils.getColorAttribsFromNode(&node, &attribs->m_colorAttribs);

    return valid;
}

bool ADFUtils::getSurfaceAttribsFromNode(YAML::Node *a_node, afSurfaceAttributes *attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node linDampingNode = node["damping"]["linear"];
    YAML::Node angDampingNode = node["damping"]["angular"];
    YAML::Node staticFrictionNode = node["friction"]["static"];
    YAML::Node rollingFrictionNode = node["friction"]["rolling"];
    YAML::Node restitutionNode = node["restitution"];

    bool valid = true;

    if (linDampingNode.IsDefined()){
        attribs->m_linearDamping = linDampingNode.as<double>();
    }
    if (angDampingNode.IsDefined()){
        attribs->m_angularDamping = angDampingNode.as<double>();
    }
    if (staticFrictionNode.IsDefined()){
        attribs->m_staticFriction = staticFrictionNode.as<double>();
    }
    if (rollingFrictionNode.IsDefined()){
        attribs->m_rollingFriction = rollingFrictionNode.as<double>();
    }
    if (restitutionNode.IsDefined()){
        attribs->m_restitution = restitutionNode.as<double>();
    }

    return valid;
}

bool ADFUtils::getWheelAttribsFromNode(YAML::Node *a_node, afWheelAttributes *attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node rigidBodyNameNode = node["body"];
    YAML::Node meshNameNode = node["mesh"];
    YAML::Node widthNode = node["width"];
    YAML::Node radiusNode = node["radius"];
    YAML::Node frictionNode = node["friction"];
    YAML::Node suspensionNode = node["suspension"];
    YAML::Node rollInfluenceNode = node["roll influence"];
    YAML::Node downDirNode = node["down direction"];
    YAML::Node axelDirNode = node["axel direction"];
    YAML::Node offsetNode = node["offset"];
    YAML::Node frontNode = node["front"];
    YAML::Node steeringLimitsNode = node["steering limits"];
    YAML::Node maxEnginePowerNode = node["max engine power"];
    YAML::Node maxBrakePowerNode = node["max brake power"];

    bool valid = true;

    attribs->m_representationType = afWheelRepresentationType::INVALID;

    if (rigidBodyNameNode.IsDefined()){
        attribs->m_wheelBodyName = rigidBodyNameNode.as<string>();
        attribs->m_representationType = afWheelRepresentationType::RIGID_BODY;
    }
    else if (meshNameNode.IsDefined()){
        attribs->m_visualAttribs.m_meshFilepath = meshNameNode.as<string>();
        attribs->m_representationType = afWheelRepresentationType::MESH;
    }
    else{
        cerr << "ERROR! UNABLE TO FIND \"MESH\" OR \"BODY\" FIELD FOR WHEEL OF VEHICLE. SKIPPING WHEEL!" << endl;
        return false;
    }


    if (widthNode.IsDefined()){
        attribs->m_width = widthNode.as<double>();
    }

    if (radiusNode.IsDefined()){
        attribs->m_radius = radiusNode.as<double>();
    }

    if (frictionNode.IsDefined()){
        attribs->m_friction = frictionNode.as<double>();
    }

    if (suspensionNode.IsDefined()){
        attribs->m_suspensionAttribs.m_stiffness = suspensionNode["stiffness"].as<double>();
        attribs->m_suspensionAttribs.m_damping = suspensionNode["damping"].as<double>();
        attribs->m_suspensionAttribs.m_compression = suspensionNode["compression"].as<double>();
        attribs->m_suspensionAttribs.m_restLength = suspensionNode["rest length"].as<double>();
    }

    if (rollInfluenceNode.IsDefined()){
        attribs->m_rollInfluence = rollInfluenceNode.as<double>();
    }

    if (downDirNode.IsDefined()){
        attribs->m_downDirection = positionFromNode(&downDirNode);
    }

    if (axelDirNode.IsDefined()){
       attribs->m_axelDirection = positionFromNode(&axelDirNode);
    }

    if (offsetNode.IsDefined()){
        attribs->m_offset = positionFromNode(&offsetNode);
    }

    if (frontNode.IsDefined()){
        attribs->m_isFront = frontNode.as<bool>();
    }

    if (steeringLimitsNode.IsDefined()){
        attribs->m_steeringLimitMax = steeringLimitsNode["high"].as<double>();
        attribs->m_steeringLimitMin = steeringLimitsNode["low"].as<double>();
    }

    if (maxEnginePowerNode.IsDefined()){
        attribs->m_enginePowerMax = maxEnginePowerNode.as<double>();
    }

    if (maxBrakePowerNode.IsDefined()){
        attribs->m_brakePowerMax = maxBrakePowerNode.as<double>();
    }

    return valid;
}

afActuatorType ADFUtils::getActuatorTypeFromString(const string &a_str)
{
    afActuatorType type = afActuatorType::INVALID;

    if (a_str.compare("Constraint") == 0 || a_str.compare("constraint") == 0 || a_str.compare("CONSTRAINT") == 0){
        type = afActuatorType::CONSTRAINT;
    }

    return type;
}

afJointType ADFUtils::getJointTypeFromString(const string &a_joint_str)
{
    afJointType jointType;

    if ((strcmp(a_joint_str.c_str(), "hinge") == 0)
            || (strcmp(a_joint_str.c_str(), "revolute") == 0)
            || (strcmp(a_joint_str.c_str(), "continuous") == 0)){
        jointType = afJointType::REVOLUTE;
    }
    else if ((strcmp(a_joint_str.c_str(), "slider") == 0)
             || (strcmp(a_joint_str.c_str(), "prismatic") == 0)){
        jointType = afJointType::PRISMATIC;
    }
    else if ((strcmp(a_joint_str.c_str(), "fixed") == 0)){
        jointType = afJointType::FIXED;
    }
    else if ((strcmp(a_joint_str.c_str(), "spring") == 0)){
        jointType = afJointType::LINEAR_SPRING;
    }
    else if ((strcmp(a_joint_str.c_str(), "linear spring") == 0)){
        jointType = afJointType::LINEAR_SPRING;
    }
    else if ((strcmp(a_joint_str.c_str(), "torsion spring") == 0)
             || (strcmp(a_joint_str.c_str(), "torsional spring") == 0)
             || (strcmp(a_joint_str.c_str(), "angular spring") == 0)){
        jointType = afJointType::TORSION_SPRING;
    }
    else if ((strcmp(a_joint_str.c_str(), "p2p") == 0)){
        jointType = afJointType::P2P;
    }
    else{
        cerr << "ERROR! JOINT TYPE NOT UNDERSTOOD \n";
        jointType = afJointType::INVALID;
    }

    return jointType;
}

afSensorType ADFUtils::getSensorTypeFromString(const string &a_str)
{
    afSensorType type = afSensorType::INVALID;
    if (a_str.compare("Proximity") == 0 || a_str.compare("proximity") == 0 || a_str.compare("PROXIMITY") == 0){
        type = afSensorType::RAYTRACER;
    }
    else if (a_str.compare("Resistance") == 0 || a_str.compare("resistance") == 0 || a_str.compare("RESISTANCE") == 0){
        type = afSensorType::RESISTANCE;
    }

    return type;
}


///
/// \brief afUtils::getCartControllerFromNode
/// \param a_node
/// \return
///
bool ADFUtils::getCartControllerAttribsFromNode(YAML::Node *a_node, afCartesianControllerAttributes* attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node controllerNode = node["controller"];

    if(controllerNode.IsDefined()){
        double P, I, D;
        // Check if the linear controller is defined
        if (controllerNode["linear"].IsDefined()){
            P = controllerNode["linear"]["P"].as<double>();
            // For legacy where we didn't define the I term
            if (controllerNode["linear"]["I"].IsDefined()){
                I = controllerNode["linear"]["I"].as<double>();
            }
            else{
                I = 0;
            }
            D = controllerNode["linear"]["D"].as<double>();
            attribs->m_positionOutputType = afControlType::FORCE;
        }
        else{
            // Use preset values for the controller since we are going to be using its output for the
            // internal velocity controller
            P = 10;
            I = 0;
            D = 0;
            attribs->m_positionOutputType = afControlType::VELOCITY;
        }
        attribs->P_lin = P;
        attribs->I_lin = I;
        attribs->D_lin = D;

        // Check if the angular controller is defined
        if(controllerNode["angular"].IsDefined()){
            P = controllerNode["angular"]["P"].as<double>();
            // For legacy where we didn't define the I term
            if (controllerNode["angular"]["I"].IsDefined()){
                I = controllerNode["angular"]["I"].as<double>();
            }
            else{
                I = 0;
            }
            D = controllerNode["angular"]["D"].as<double>();
            attribs->m_orientationOutputType = afControlType::FORCE;
        }
        else{
            // Use preset values for the controller since we are going to be using its output for the
            // internal velocity controller
            P = 10;
            I = 0;
            D = 0;
            attribs->m_orientationOutputType = afControlType::VELOCITY;
        }
        attribs->P_ang = P;
        attribs->I_ang = I;
        attribs->D_ang = D;
    }
    else{
        return false;
    }

    return true;
}

bool ADFUtils::getCollisionAttribsFromNode(YAML::Node *a_node, afCollisionAttributes *attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node collisionMarginNode = node["collision margin"];
    YAML::Node collisionGroupsNode = node["collision groups"];

    YAML::Node meshNode = node["mesh"];
    YAML::Node collisionMeshNode = node["collision mesh"];
    YAML::Node collisionShapeNode = node["collision shape"];
    YAML::Node collisionOffsetNode = node["collision offset"];
    YAML::Node collisionGeometryNode = node["collision geometry"];
    YAML::Node compoundCollisionShapeNode = node["compound collision shape"];

    YAML::Node meshPathHRNode = node["high resolution path"];
    YAML::Node meshPathLRNode = node["low resolution path"];

    // For backward compatibility in ADF 1.0, if the collision offset is not defined, we set the
    // collision offset as the inertial offset for single mesh or single collision shape.
    YAML::Node inertialOffsetNode = node["inertial offset"];

    if (collisionOffsetNode.IsDefined() == false){
        collisionOffsetNode = inertialOffsetNode;
    }

    bool valid = true;
    string shape_str;

    attribs->m_geometryType = afGeometryType::INVALID;

    if (collisionMarginNode.IsDefined()){
        attribs->m_margin = collisionMarginNode.as<double>();
    }

    if (collisionGroupsNode.IsDefined()){
        for (uint gIdx = 0 ; gIdx < collisionGroupsNode.size() ; gIdx++){
            int gNum = collisionGroupsNode[gIdx].as<int>();
            // Sanity check for the group number
            if (gNum >= 0 && gNum <= 999){
                attribs->m_groups.push_back(gNum);
            }
            else{
                cerr << "WARNING: Body Collision group number is \""
                          << gNum
                          << "\" but should be between [0 - 999], ignoring\n";
            }
        }
    }

    afPath localPath;

    if (meshPathLRNode.IsDefined()){
        localPath = meshPathLRNode.as<string>();
    }
    else if(meshPathHRNode.IsDefined()){
        localPath = meshPathHRNode.as<string>();
    }

    if(collisionShapeNode.IsDefined()){
        attribs->m_geometryType = afGeometryType::SINGLE_SHAPE;
        shape_str = collisionShapeNode.as<string>();
        afPrimitiveShapeAttributes shapeAttribs;
        shapeAttribs.setShapeType(ADFUtils::getShapeTypeFromString(shape_str));
        ADFUtils::copyPrimitiveShapeData(&collisionGeometryNode, &shapeAttribs);
        ADFUtils::copyShapeOffsetData(&collisionOffsetNode, &shapeAttribs);
        attribs->m_primitiveShapes.push_back(shapeAttribs);
    }
    else if(compoundCollisionShapeNode.IsDefined()){
        attribs->m_geometryType = afGeometryType::COMPOUND_SHAPE;
        for (uint shapeIdx = 0 ; shapeIdx < compoundCollisionShapeNode.size() ; shapeIdx++){
            shape_str = compoundCollisionShapeNode[shapeIdx]["shape"].as<string>();
            YAML::Node collisionGeometryNode = compoundCollisionShapeNode[shapeIdx]["geometry"];
            YAML::Node shapeOffsetNode = compoundCollisionShapeNode[shapeIdx]["offset"];

            afPrimitiveShapeAttributes shapeAttribs;
            shapeAttribs.setShapeType(ADFUtils::getShapeTypeFromString(shape_str));
            ADFUtils::copyPrimitiveShapeData(& collisionGeometryNode, &shapeAttribs);
            ADFUtils::copyShapeOffsetData(&shapeOffsetNode, &shapeAttribs);
            attribs->m_primitiveShapes.push_back(shapeAttribs);
        }
    }
    else if (meshNode.IsDefined()){
        attribs->m_meshFilepath = localPath / meshNode.as<string>();
        if (!attribs->m_meshFilepath.c_str().empty()){
            attribs->m_geometryType = afGeometryType::MESH;
        }
        else{
            valid = false;
        }
    }
    else if (collisionMeshNode.IsDefined()){
        attribs->m_meshFilepath = localPath / collisionMeshNode.as<string>();
        if (!attribs->m_meshFilepath.c_str().empty()){
            attribs->m_geometryType = afGeometryType::MESH;
        }
        else{
            valid = false;
        }
    }
    else{
        valid = false;
    }

    return valid;
}

bool ADFUtils::getCommunicationAttribsFromNode(YAML::Node *a_node, afCommunicationAttributes *attribs)
{
    YAML::Node& node = *a_node;
    YAML::Node publishFrequencyNode = node["publish frequency"];
    YAML::Node passiveNode = node["passive"];

    bool valid = true;

    if (publishFrequencyNode.IsDefined()){
        attribs->m_minPublishFreq = publishFrequencyNode["low"].as<uint>();
        attribs->m_maxPublishFreq = publishFrequencyNode["high"].as<uint>();
    }

    if (passiveNode.IsDefined()){
        attribs->m_passive = passiveNode.as<bool>();
    }

    return valid;
}

bool ADFUtils::getHierarchyAttribsFromNode(YAML::Node *a_node, afHierarchyAttributes *attribs)
{
    YAML::Node &node = *a_node;

    YAML::Node parentNameNode = node["parent"];
    YAML::Node childNameNode = node["child"];

    bool valid = true;

    if (childNameNode.IsDefined()){
//        string name = childNameNode.as<string>();
//        name.erase(remove(name.begin(), name.end(), ' '), name.end());
//        attribs->m_childName = name;

        valid = true;
        attribs->m_childName = childNameNode.as<string>();
    }
    else{
        valid = false;
    }

    if (parentNameNode.IsDefined()){
//        string name = parentNameNode.as<string>();
//        name.erase(remove(name.begin(), name.end(), ' '), name.end());
//        attribs->m_parentName = name;

        valid = true;
        attribs->m_parentName = parentNameNode.as<string>();
    }
    else{
        valid = false;
    }

    return valid;
}

bool ADFUtils::getIdentificationAttribsFromNode(YAML::Node *a_node, afIdentificationAttributes *attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];

    bool valid;

    if(nameNode.IsDefined()){
        string name = nameNode.as<string>();
        name.erase(remove(name.begin(), name.end(), ' '), name.end());
        attribs->m_name = name;
        valid = true;
    }

    if (namespaceNode.IsDefined()){
        attribs->m_namespace = namespaceNode.as<string>();
        valid = true;
    }

    return valid;
}

bool ADFUtils::getInertialAttrisFromNode(YAML::Node *a_node, afInertialAttributes *attribs)
{
    YAML::Node &node = *a_node;

    YAML::Node massNode = node["mass"];
    YAML::Node inertiaNode = node["inertia"];
    YAML::Node inertialOffset = node["inertial offset"];

    bool valid = true;

    if(!massNode.IsDefined()){
        cerr << "WARNING: Body's mass is not defined, ignoring!\n";
        return false;
    }
    else{
        attribs->m_mass = massNode.as<double>();
    }

    if(inertiaNode.IsDefined()){
        double ix = inertiaNode["ix"].as<double>();
        double iy = inertiaNode["ix"].as<double>();
        double iz = inertiaNode["ix"].as<double>();

        attribs->m_inertia.set(ix, iy, iz);
        attribs->m_estimateInertia = false;
    }
    else{
        attribs->m_estimateInertia = true;
    }

    if(inertialOffset.IsDefined()){
        YAML::Node inertialOffsetPos = inertialOffset["position"];
        YAML::Node inertialOffsetRot = inertialOffset["orientation"];
        attribs->m_estimateInertialOffset = false;

        if (inertialOffsetPos.IsDefined()){
            afVector3d pos = ADFUtils::positionFromNode(&inertialOffsetPos);
            attribs->m_inertialOffset.setPosition(pos);
        }

        if (inertialOffsetRot.IsDefined()){
            afMatrix3d rot = ADFUtils::rotationFromNode(&inertialOffsetRot);
            attribs->m_inertialOffset.setRotation(rot);
        }
    }
    else{
        attribs->m_estimateInertialOffset = true;
    }

    return valid;
}

bool ADFUtils::getJointControllerAttribsFromNode(YAML::Node *a_node, afJointControllerAttributes *attribs)
{
    YAML::Node & node = *a_node;
    YAML::Node controllerNode = node["controller"];

    if (controllerNode.IsDefined()){
        if( (controllerNode["P"]).IsDefined())
            attribs->m_P = controllerNode["P"].as<double>();
        if( (controllerNode["I"]).IsDefined())
            attribs->m_I = controllerNode["I"].as<double>();
        if( (controllerNode["D"]).IsDefined())
            attribs->m_D = controllerNode["D"].as<double>();

        // If the PID controller in defined, the gains will be used to command the joint force (effort)
        attribs->m_outputType = afControlType::FORCE;
    }
    else{
        // If the controller gains are not defined, a velocity based control will be used.
        // The tracking velocity can be controller by setting "max motor impulse" field
        // for the joint data-block in the ADF file.
        attribs->m_P = 10;
        attribs->m_I = 0;
        attribs->m_D = 0;
        attribs->m_outputType = afControlType::VELOCITY;
    }

    return true;
}

bool ADFUtils::getKinematicAttribsFromNode(YAML::Node *a_node, afKinematicAttributes *attribs)
{
    YAML::Node& node = *a_node;

    YAML::Node posNode = node["location"]["position"];
    YAML::Node rotNode = node["location"]["orientation"];
    YAML::Node scaleNode = node["scale"];

    bool valid = true;

    if(scaleNode.IsDefined()){
        attribs->m_scale = scaleNode.as<double>();
    }

    if(posNode.IsDefined()){
        afVector3d pos = ADFUtils::positionFromNode(&posNode);
        attribs->m_location.setPosition(pos);
    }
    else{
        valid = false;
    }

    if(rotNode.IsDefined()){
        afMatrix3d rot = ADFUtils::rotationFromNode(&rotNode);
        attribs->m_location.setRotation(rot);
    }
    else{
        valid = false;
    }

    return valid;
}


///
/// \brief ADFUtils::getShapeTypeFromString
/// \param a_shape_str
/// \return
///
afPrimitiveShapeType ADFUtils::getShapeTypeFromString(const string &a_shape_str){
    afPrimitiveShapeType shapeType;
    if (a_shape_str.compare("Box") == 0 || a_shape_str.compare("box") == 0 || a_shape_str.compare("BOX") == 0){
        shapeType = afPrimitiveShapeType::BOX;
    }
    else if (a_shape_str.compare("Sphere") == 0 || a_shape_str.compare("sphere") == 0 || a_shape_str.compare("SPHERE") == 0){
        shapeType = afPrimitiveShapeType::SPHERE;
    }
    else if (a_shape_str.compare("Cylinder") == 0 || a_shape_str.compare("cylinder") == 0 || a_shape_str.compare("CYLINDER") == 0){
        shapeType = afPrimitiveShapeType::CYLINDER;
    }
    else if (a_shape_str.compare("Capsule") == 0 || a_shape_str.compare("capsule") == 0 || a_shape_str.compare("CAPSULE") == 0){
        shapeType = afPrimitiveShapeType::CAPSULE;
    }
    else if (a_shape_str.compare("Cone") == 0 || a_shape_str.compare("cone") == 0 || a_shape_str.compare("CONE") == 0){
        shapeType = afPrimitiveShapeType::CONE;
    }
    else if (a_shape_str.compare("Plane") == 0 || a_shape_str.compare("plane") == 0 || a_shape_str.compare("PLANE") == 0){
        shapeType = afPrimitiveShapeType::PLANE;

    }
    else{
        shapeType = afPrimitiveShapeType::INVALID;
    }

    return shapeType;
}


///
/// \brief ADFUtils::copyShapeOffsetData
/// \param offset_node
/// \param attribs
/// \return
///
bool ADFUtils::copyShapeOffsetData(YAML::Node *offset_node, afPrimitiveShapeAttributes* attribs)
{
    bool valid = true;

    YAML::Node offsetNode = *offset_node;

    if (offsetNode.IsDefined()){
        if (offsetNode["position"].IsDefined()){
            YAML::Node posNode = offsetNode["position"];
            attribs->m_offset.setPosition(ADFUtils::positionFromNode(&posNode));
        }
        else{
            attribs->m_offset.setPosition(afVector3d(0, 0, 0));
        }

        if (offsetNode["orientation"].IsDefined()){
            YAML::Node orientationNode = offsetNode["orientation"];
            attribs->m_offset.setRotation(ADFUtils::rotationFromNode(&orientationNode));
        }
        else{
            attribs->m_offset.setRotation(afMatrix3d(0, 0, 0));
        }
    }
    else{
        valid = false;
    }

    return valid;
}


///
/// \brief ADFUtils::copyPrimitiveShapeData
/// \param shape_node
/// \param attribs
/// \return
///
bool ADFUtils::copyPrimitiveShapeData(YAML::Node *shape_node, afPrimitiveShapeAttributes* attribs)
{
    bool valid = true;

    YAML::Node shapeNode = *shape_node;

    if(shapeNode["axis"].IsDefined()){
        string axis = shapeNode["axis"].as<string>();

        if (axis.compare("x") == 0 || axis.compare("X") == 0){
            attribs->m_axisType = afAxisType::X;
        }
        else if (axis.compare("y") == 0 || axis.compare("Y") == 0){
            attribs->m_axisType = afAxisType::Y;
        }
        else if (axis.compare("z") == 0 || axis.compare("Z") == 0){
            attribs->m_axisType = afAxisType::Z;
        }
        else{
            cerr << "WARNING: Axis string \"" << axis << "\" not understood!\n";
            attribs->m_axisType = afAxisType::Z;
        }
    }

    switch (attribs->m_shapeType) {
    case afPrimitiveShapeType::BOX:{
        double dx, dy, dz;
        dx = shapeNode["x"].as<double>();
        dy = shapeNode["y"].as<double>();
        dz = shapeNode["z"].as<double>();
        attribs->m_dimensions.set(dx, dy, dz);
        break;
    }
    case afPrimitiveShapeType::SPHERE:{
        attribs->m_radius = shapeNode["radius"].as<double>();
        break;
    }
    case afPrimitiveShapeType::CAPSULE:{
        attribs->m_radius = shapeNode["radius"].as<double>();
        attribs->m_height = shapeNode["height"].as<double>();
        break;
    }
    case afPrimitiveShapeType::CYLINDER:{
        attribs->m_radius = shapeNode["radius"].as<double>();
        attribs->m_height = shapeNode["height"].as<double>();
        break;
    }
    case afPrimitiveShapeType::CONE:{
        attribs->m_radius = shapeNode["radius"].as<double>();
        attribs->m_height = shapeNode["height"].as<double>();
        break;
    }
    case afPrimitiveShapeType::PLANE:{
        double nx, ny, nz;
        nx = shapeNode["normal"]["x"].as<double>();
        ny = shapeNode["normal"]["y"].as<double>();
        nz = shapeNode["normal"]["z"].as<double>();
        attribs->m_planeNormal.set(nx, ny, nz);
        attribs->m_planeConstant = shapeNode["offset"].as<double>();
        break;
    }
    default:{
        valid = false;
        break;
    }
    }

    return valid;
}


ADFLoader_1_0::ADFLoader_1_0()
{
    m_version = "1.0";
}

string ADFLoader_1_0::getLoaderVersion(){
    return m_version;
}

bool ADFLoader_1_0::loadObjectAttribs(YAML::Node *a_node, string a_objName, afObjectType a_objType, afBaseObjectAttributes *attribs)
{
    YAML::Node& rootNode = *a_node;
    if (rootNode.IsNull()){
        cerr << "ERROR: OBJECT'S YAML NODE IS NULL\n";
        return 0;
    }

    YAML::Node node = rootNode[a_objName];
    switch (a_objType) {
    case afObjectType::CONSTRAINT_ACTUATOR:
        return loadConstraintActuatorAttribs(&node, (afConstraintActuatorAttributes*)attribs);
    case afObjectType::RAYTRACER_SENSOR:
        return loadRayTracerSensorAttribs(&node, (afRayTracerSensorAttributes*)attribs);
    case afObjectType::RESISTANCE_SENSOR:
        return loadResistanceSensorAttribs(&node, (afResistanceSensorAttributes*)attribs);
    case afObjectType::RIGID_BODY:
        return loadRigidBodyAttribs(&node, (afRigidBodyAttributes*)attribs);
    case afObjectType::SOFT_BODY:
        return loadSoftBodyAttribs(&node, (afSoftBodyAttributes*)attribs);
    case afObjectType::VEHICLE:
        return loadVehicleAttribs(&node, (afVehicleAttributes*)attribs);
    case afObjectType::LIGHT:
        return loadLightAttribs(&node, (afLightAttributes*)attribs);
    case afObjectType::CAMERA:
        return loadCameraAttribs(&node, (afCameraAttributes*)attribs);
    case afObjectType::INPUT_DEVICE:
        return loadInputDeviceAttributes(&node, (afInputDeviceAttributes*)attribs);
    default:
        return false;
    }
}

bool ADFLoader_1_0::loadLightAttribs(YAML::Node *a_node, afLightAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: LIGHT'S " << node << " NODE IS NULL\n";
        return false;
    }

    if (attribs == nullptr){
        cerr << "ERROR: LIGHT'S ATTRIBUTES IS A NULLPTR\n";
        return false;
    }

    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];
    YAML::Node parentNode = node["parent"];
    YAML::Node locationNode = node["location"];
    YAML::Node directionNode = node["direction"];
    YAML::Node spotExponentNode = node["spot exponent"];
    YAML::Node shadowQualityNode = node["shadow quality"];
    YAML::Node cuttOffAngleNode = node["cutoff angle"];

    bool valid = true;

    afVector3d location, direction;
    double spot_exponent, cuttoff_angle;
    int shadow_quality;

    ADFUtils adfUtils;

    node["location"]["position"] = node["location"];
    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getHierarchyAttribsFromNode(&node, &attribs->m_hierarchyAttribs);
    adfUtils.getKinematicAttribsFromNode(&node, &attribs->m_kinematicAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);

    if (directionNode.IsDefined()){
        attribs->m_direction = ADFUtils::positionFromNode(&directionNode);
    }

    if (spotExponentNode.IsDefined()){
        attribs->m_spotExponent = spotExponentNode.as<double>();
    }

    if (shadowQualityNode.IsDefined()){
        uint quality = shadowQualityNode.as<uint>();
        switch (quality) {
        case 0:
            attribs->m_shadowQuality = afShadowQualityType::NO_SHADOW;
            break;
        case 1:
            attribs->m_shadowQuality = afShadowQualityType::VERY_LOW;
            break;
        case 2:
            attribs->m_shadowQuality = afShadowQualityType::LOW;
            break;
        case 3:
            attribs->m_shadowQuality = afShadowQualityType::MEDIUM;
            break;
        case 4:
            attribs->m_shadowQuality = afShadowQualityType::HIGH;
            break;
        case 5:
            attribs->m_shadowQuality = afShadowQualityType::VERY_HIGH;
            break;
        default:
            break;
        }
    }

    if (cuttOffAngleNode.IsDefined()){
        attribs->m_cuttoffAngle = cuttOffAngleNode.as<double>();
    }

    return valid;
}

bool ADFLoader_1_0::loadCameraAttribs(YAML::Node *a_node, afCameraAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: CAMERA'S " << node << " NODE IS NULL\n";
        return false;
    }

    if (attribs == nullptr){
        cerr << "ERROR: CAMERA'S ATTRIBUTES IS A NULLPTR\n";
        return false;
    }

    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];
    YAML::Node parentNode = node["parent"];
    YAML::Node locationNode = node["location"];
    YAML::Node lookAtNode = node["look at"];
    YAML::Node upNode = node["up"];
    YAML::Node clippingPlaneNode = node["clipping plane"];
    YAML::Node fieldViewAngleNode = node["field view angle"];
    YAML::Node orthoWidthNode = node["orthographic view width"];
    YAML::Node stereoNode = node["stereo"];
    YAML::Node controllingDevicesDataNode = node["controlling devices"];
    YAML::Node monitorNode = node["monitor"];
    YAML::Node publishImageNode = node["publish image"];
    YAML::Node publishImageIntervalNode = node["publish image interval"];
    YAML::Node publishDepthNode = node["publish depth"];
    YAML::Node publishDepthIntervalNode = node["publish depth interval"];
    YAML::Node multiPassNode = node["multipass"];

    bool valid = true;

    ADFUtils adfUtils;

    node["location"]["position"] = node["location"];
    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getKinematicAttribsFromNode(&node, &attribs->m_kinematicAttribs);
    adfUtils.getHierarchyAttribsFromNode(&node, &attribs->m_hierarchyAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);

    if (lookAtNode.IsDefined()){
        attribs->m_lookAt = ADFUtils::positionFromNode(&lookAtNode);
    }

    if (upNode.IsDefined()){
        attribs->m_up = ADFUtils::positionFromNode(&upNode);
    }

    if (clippingPlaneNode.IsDefined()){
        attribs->m_nearPlane = clippingPlaneNode["near"].as<double>();
        attribs->m_farPlane = clippingPlaneNode["far"].as<double>();
    }

    if (fieldViewAngleNode.IsDefined()){
        attribs->m_fieldViewAngle = fieldViewAngleNode.as<double>();
    }

    if (orthoWidthNode.IsDefined()){
        attribs->m_orthographic = true;
        attribs->m_orthoViewWidth = orthoWidthNode.as<double>();
    }

    if (stereoNode.IsDefined()){
        string mode_str = stereoNode["mode"].as<string>();
        if (mode_str.compare("PASSIVE") || mode_str.compare("passive") || mode_str.compare("Passive")){
            attribs->m_stereo = true;
        }
        attribs->m_stereoEyeSeparation = stereoNode["eye separation"].as<double>();
        attribs->m_stereFocalLength = stereoNode["focal length"].as<double>();
    }

    if (monitorNode.IsDefined()){
        attribs->m_monitorNumber = monitorNode.as<uint>();
    }

    if (controllingDevicesDataNode.IsDefined()){
        for(uint idx = 0 ; idx < controllingDevicesDataNode.size() ; idx++){
            attribs->m_controllingDeviceNames.push_back( controllingDevicesDataNode[idx].as<string>());
        }
    }

    if (publishImageNode.IsDefined()){
        attribs->m_publishImage = publishImageNode.as<bool>();
    }

    if (publishImageIntervalNode.IsDefined()){
        attribs->m_publishImageInterval = publishImageIntervalNode.as<uint>();
    }

    if (publishDepthNode.IsDefined()){
        attribs->m_publishDepth = publishDepthNode.as<bool>();
    }

    if (publishDepthIntervalNode.IsDefined()){
        attribs->m_publishDepthInterval = publishDepthIntervalNode.as<uint>();
    }

    if (multiPassNode.IsDefined()){
        attribs->m_multiPass = multiPassNode.as<bool>();
    }

    return valid;
}

bool ADFLoader_1_0::loadRigidBodyAttribs(YAML::Node *a_node, afRigidBodyAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: RIGID BODY " << node << " NODE IS NULL\n";
        return false;
    }

    if (attribs == nullptr){
        cerr << "ERROR: RIGID BODY ATTRIBUTES IS A NULLPTR\n";
        return false;
    }
    // Declare all the yaml parameters that we want to look for
    // IDENTIFICATION
    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];
    // KINEMATICS
    YAML::Node posNode = node["location"]["position"];
    YAML::Node rotNode = node["location"]["orientation"];
    YAML::Node scaleNode = node["scale"];
    // INERTIAL
    YAML::Node massNode = node["mass"];
    YAML::Node inertiaNode = node["inertia"];
    YAML::Node inertialOffsetPosNode = node["inertial offset"]["position"];
    YAML::Node inertialOffsetRotNode = node["inertial offset"]["orientation"];
    // VISUAL
    YAML::Node meshPathHRNode = node["high resolution path"];
    YAML::Node meshPathLRNode = node["low resolution path"];
    YAML::Node meshNode = node["mesh"];
    YAML::Node shapeNode = node["shape"];
    YAML::Node compoundShapeNode = node["compound shape"];
    YAML::Node geometryNode = node["geometry"];
    YAML::Node shadersNode = node["shaders"];
    YAML::Node colorNode = node["color"];
    YAML::Node colorRGBANode = node["color rgba"];
    YAML::Node colorComponentsNode = node["color components"];
    // COLLISION
    YAML::Node collisionMarginNode = node["collision margin"];
    YAML::Node collisionMeshNode = node["collision mesh"];
    YAML::Node collisionShapeNode = node["collision shape"];
    YAML::Node collisionOffsetNode = node["collision offset"];
    YAML::Node collisionGeometryNode = node["collision geometry"];
    YAML::Node compoundCollisionShapeNode = node["compound collision shape"];
    YAML::Node collisionGroupsNode = node["collision groups"];
    // CONTROLLERS
    YAML::Node controllerNode = node["controller"];
    // SURFACE PROPS
    YAML::Node linDampingNode = node["damping"]["linear"];
    YAML::Node angDampingNode = node["damping"]["angular"];
    YAML::Node staticFrictionNode = node["friction"]["static"];
    YAML::Node rollingFrictionNode = node["friction"]["rolling"];
    YAML::Node restitutionNode = node["restitution"];
    // COMMUNICATION
    YAML::Node publishFrequencyNode = node["publish frequency"];
    YAML::Node passiveNode = node["passive"];
    // Rigid Body Specific
    YAML::Node publishChildrenNamesNode = node["publish children names"];
    YAML::Node publishJointNamesNode = node["publish joint names"];
    YAML::Node publishJointPositionsNode = node["publish joint positions"];

    ADFUtils adfUtils;

    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getVisualAttribsFromNode(&node, &attribs->m_visualAttribs);
    adfUtils.getKinematicAttribsFromNode(&node, &attribs->m_kinematicAttribs);
    adfUtils.getCollisionAttribsFromNode(&node, &attribs->m_collisionAttribs);
    adfUtils.getInertialAttrisFromNode(&node, &attribs->m_inertialAttribs);
    adfUtils.getCartControllerAttribsFromNode(&node, &attribs->m_controllerAttribs);
    adfUtils.getSurfaceAttribsFromNode(&node, &attribs->m_surfaceAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);
    adfUtils.getShaderAttribsFromNode(&node, &attribs->m_shaderAttribs);

    if (publishChildrenNamesNode.IsDefined()){
        attribs->m_publishChildrenNames = publishChildrenNamesNode.as<bool>();
    }

    if (publishJointNamesNode.IsDefined()){
        attribs->m_publishJointNames = publishJointNamesNode.as<bool>();
    }

    if (publishJointPositionsNode.IsDefined()){
        attribs->m_publishJointPositions = publishJointPositionsNode.as<bool>();
    }

    return true;
}

bool ADFLoader_1_0::loadSoftBodyAttribs(YAML::Node *a_node, afSoftBodyAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: SOFT BODY'S YAML NODE IS NULL\n";
        return 0;
    }
    // Declare all the yaml parameters that we want to look for
    YAML::Node nameNode = node["name"];
    YAML::Node meshNode = node["mesh"];
    YAML::Node collisionMarginNode = node["collision margin"];
    YAML::Node scaleNode = node["scale"];
    YAML::Node inertialOffsetPosNode = node["inertial offset"]["position"];
    YAML::Node inertialOffsetRotNode = node["inertial offset"]["orientation"];
    YAML::Node meshPathHRNode = node["high resolution path"];
    YAML::Node meshPathLRNode = node["low resolution path"];
    YAML::Node mameSpaceNode = node["namespace"];
    YAML::Node massNode = node["mass"];
    YAML::Node linGainNode = node["linear gain"];
    YAML::Node angGainNode = node["angular gain"];
    YAML::Node posNode = node["location"]["position"];
    YAML::Node rotNode = node["location"]["orientation"];
    YAML::Node colorNode = node["color"];
    YAML::Node colorRGBANode = node["color rgba"];
    YAML::Node colorComponentsNode = node["color components"];

    YAML::Node configDataNode = node["config"];

    YAML::Node cfg_kLSTNode = configDataNode["kLST"];
    YAML::Node cfg_kASTNode = configDataNode["kAST"];
    YAML::Node cfg_kVSTNode = configDataNode["kVST"];
    YAML::Node cfg_kVCFNode = configDataNode["kVCF"];
    YAML::Node cfg_kDPNode = configDataNode["kDP"];
    YAML::Node cfg_kDGNode = configDataNode["kDG"];
    YAML::Node cfg_kLFNode = configDataNode["kLF"];
    YAML::Node cfg_kPRNode = configDataNode["kPR"];
    YAML::Node cfg_kVCNode = configDataNode["kVC"];
    YAML::Node cfg_kDFNode = configDataNode["kDF"];
    YAML::Node cfg_kMTNode = configDataNode["kMT"];
    YAML::Node cfg_kCHRNode = configDataNode["kCHR"];
    YAML::Node cfg_kKHRNode = configDataNode["kKHR"];
    YAML::Node cfg_kSHRNode = configDataNode["kSHR"];
    YAML::Node cfg_kAHRNode = configDataNode["kAHR"];
    YAML::Node cfg_kSRHR_CLNode = configDataNode["kSRHR_CL"];
    YAML::Node cfg_kSKHR_CLNode = configDataNode["kSKHR_CL"];
    YAML::Node cfg_kSSHR_CLNode = configDataNode["kSSHR_CL"];
    YAML::Node cfg_kSR_SPLT_CLNode = configDataNode["kSR_SPLT_CL"];
    YAML::Node cfg_kSK_SPLT_CLNode = configDataNode["kSK_SPLT_CL"];
    YAML::Node cfg_kSS_SPLT_CLNode = configDataNode["kSS_SPLT_CL"];
    YAML::Node cfg_maxvolumeNode = configDataNode["maxvolume"];
    YAML::Node cfg_timescaleNode = configDataNode["timescale"];
    YAML::Node cfg_viterationsNode = configDataNode["viterations"];
    YAML::Node cfg_piterationsNode = configDataNode["piterations"];
    YAML::Node cfg_diterationsNode = configDataNode["diterations"];
    YAML::Node cfg_citerationsNode = configDataNode["citerations"];
    YAML::Node cfg_flagsNode = configDataNode["flags"];
    YAML::Node cfg_bendingConstraintNode = configDataNode["bending constraint"];
    YAML::Node cfg_cuttingNode = configDataNode["cutting"];
    YAML::Node cfg_clustersNode = configDataNode["clusters"];
    YAML::Node cfg_fixed_nodesNode = configDataNode["fixed nodes"];

    YAML::Node randomizeConstraintsNode = node["randomize constraints"];

    ADFUtils adfUtils;

    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getVisualAttribsFromNode(&node, &attribs->m_visualAttribs);
    adfUtils.getCollisionAttribsFromNode(&node, &attribs->m_collisionAttribs);
    adfUtils.getKinematicAttribsFromNode(&node, &attribs->m_kinematicAttribs);
    adfUtils.getInertialAttrisFromNode(&node, &attribs->m_inertialAttribs);
    adfUtils.getCartControllerAttribsFromNode(&node, &attribs->m_controllerAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);


    if (configDataNode.IsNull()){
        printf("Warning, no soft body config properties defined");
    }
    else{
        if (cfg_kLSTNode.IsDefined()){
            attribs->m_kLST = cfg_kLSTNode.as<double>();
            attribs->m_useMaterial = true;
        }
        if (cfg_kASTNode.IsDefined()){
            attribs->m_kAST = cfg_kASTNode.as<double>();
            attribs->m_useMaterial = true;
        }
        if (cfg_kVSTNode.IsDefined()){
            attribs->m_kVST = cfg_kVSTNode.as<double>();
            attribs->m_useMaterial = true;
        }
        if (cfg_kVCFNode.IsDefined()){
            attribs->m_kVCF = cfg_kVCFNode.as<double>();
        }
        if (cfg_kDPNode.IsDefined()){
            attribs->m_kDP = cfg_kDPNode.as<double>();
        }
        if (cfg_kDGNode.IsDefined()){
            attribs->m_kDG = cfg_kDGNode.as<double>();
        }
        if (cfg_kLFNode.IsDefined()){
            attribs->m_kLF = cfg_kLFNode.as<double>();
        }
        if (cfg_kPRNode.IsDefined()) {
            attribs->m_kPR = cfg_kPRNode.as<double>();
        }
        if (cfg_kVCNode.IsDefined()) {
            attribs->m_kVC = cfg_kVCNode.as<double>();
        }
        if (cfg_kDFNode.IsDefined()) {
            attribs->m_kDF = cfg_kDFNode.as<double>();
        }
        if (cfg_kMTNode.IsDefined()){
           attribs->m_kMT = cfg_kMTNode.as<double>();
           attribs->m_usePoseMatching = true;
        }
        if (cfg_kCHRNode.IsDefined()) {
            attribs->m_kCHR = cfg_kCHRNode.as<double>();
        }
        if (cfg_kKHRNode.IsDefined()) {
           attribs->m_kKHR = cfg_kKHRNode.as<double>();
        }
        if (cfg_kSHRNode.IsDefined()) {
           attribs->m_kSHR = cfg_kSHRNode.as<double>();
        }
        if (cfg_kAHRNode.IsDefined()) {
           attribs->m_kAHR = cfg_kAHRNode.as<double>();
        }
        if (cfg_kSRHR_CLNode.IsDefined()) {
            attribs->m_kSRHR_CL = cfg_kSRHR_CLNode.as<double>();
        }
        if (cfg_kSKHR_CLNode.IsDefined()) {
            attribs->m_kSKHR_CL = cfg_kSKHR_CLNode.as<double>();
        }
        if (cfg_kSSHR_CLNode.IsDefined()) {
            attribs->m_kSSHR_CL = cfg_kSSHR_CLNode.as<double>();
        }
        if (cfg_kSR_SPLT_CLNode.IsDefined()) {
           attribs->m_kSR_SPLT_CL = cfg_kSR_SPLT_CLNode.as<double>();
        }
        if (cfg_kSK_SPLT_CLNode.IsDefined()) {
            attribs->m_kSK_SPLT_CL = cfg_kSK_SPLT_CLNode.as<double>();
        }
        if (cfg_kSS_SPLT_CLNode.IsDefined()) {
            attribs->m_kSS_SPLT_CL = cfg_kSS_SPLT_CLNode.as<double>();
        }
        if (cfg_maxvolumeNode.IsDefined()) {
            attribs->m_maxVolume = cfg_maxvolumeNode.as<double>();
        }
        if (cfg_timescaleNode.IsDefined()) {
            attribs->m_timeScale = cfg_timescaleNode.as<double>();
        }
        if (cfg_viterationsNode.IsDefined()) {
           attribs->m_vIterations = cfg_viterationsNode.as<int>();
        }
        if (cfg_piterationsNode.IsDefined()) {
           attribs->m_pIterations = cfg_piterationsNode.as<int>();
        }
        if (cfg_diterationsNode.IsDefined()) {
           attribs->m_dIterations = cfg_diterationsNode.as<int>();
        }
        if (cfg_citerationsNode.IsDefined()) {
            attribs->m_cIterations = cfg_citerationsNode.as<int>();
        }
        if (cfg_flagsNode.IsDefined()){
            attribs->m_flags = cfg_flagsNode.as<int>();
        }
        if (cfg_bendingConstraintNode.IsDefined()){
            attribs->m_bendingConstraint = cfg_bendingConstraintNode.as<int>();
            attribs->m_useBendingConstraints = true;
        }
        if (cfg_fixed_nodesNode.IsDefined()){
            for (uint i = 0 ; i < cfg_fixed_nodesNode.size() ; i++){
                attribs->m_fixedNodes.push_back(i);
            }
        }
        if(cfg_clustersNode.IsDefined()){
            attribs->m_clusters = cfg_clustersNode.as<int>();
            attribs->m_useClusters = true;
        }
    }

    if (randomizeConstraintsNode.IsDefined()){
        attribs->m_useConstraintRandomization = randomizeConstraintsNode.as<bool>();
        attribs->m_useConstraintRandomization = true;
    }

    return true;
}

bool ADFLoader_1_0::loadJointAttribs(YAML::Node *a_node, afJointAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: JOINT'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }
    // Declare all the yaml parameters that we want to look for
    YAML::Node parentNameNode = node["parent"];
    YAML::Node childNameNode = node["child"];
    YAML::Node nameNode = node["name"];
    YAML::Node parentPivotNode = node["parent pivot"];
    YAML::Node childPivotNode = node["child pivot"];
    YAML::Node parentAxisNode = node["parent axis"];
    YAML::Node childAxisNode = node["child axis"];
    YAML::Node originNode = node["origin"];
    YAML::Node axisNode = node["axis"];
    YAML::Node enableMotorNode = node["enable motor"];
    YAML::Node enableFeedbackNode = node["enable feedback"];
    YAML::Node maxMotorImpulseNode = node["max motor impulse"];
    YAML::Node limitsNode = node["joint limits"];
    YAML::Node erpNode = node["joint erp"];
    YAML::Node cfmNode = node["joint cfm"];
    YAML::Node offsetNode = node["offset"];
    YAML::Node dampingNode = node["damping"];
    YAML::Node stiffnessNode = node["stiffness"];
    YAML::Node typeNode = node["type"];
    YAML::Node controllerNode = node["controller"];
    YAML::Node ignoreInterCollisionNode = node["ignore inter-collision"];
    YAML::Node equilibriumPointNode = node["equilibrium point"];
    YAML::Node passiveNode = node["passive"];

    ADFUtils adfUtils;

    adfUtils.getHierarchyAttribsFromNode(&node, &attribs->m_hierarchyAttribs);
    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);
    adfUtils.getJointControllerAttribsFromNode(&node, &attribs->m_controllerAttribs);

    attribs->m_parentPivot = ADFUtils::positionFromNode(&parentPivotNode);
    attribs->m_childPivot = ADFUtils::positionFromNode(&childPivotNode);
    attribs->m_parentAxis = ADFUtils::positionFromNode(&parentAxisNode);
    attribs->m_childAxis = ADFUtils::positionFromNode(&childAxisNode);

    attribs->m_jointType = ADFUtils::getJointTypeFromString(typeNode.as<string>());


    if(offsetNode.IsDefined()){
        attribs->m_offset = offsetNode.as<double>();
    }

    if (dampingNode.IsDefined()){
        attribs->m_damping = dampingNode.as<double>();
    }

    if(limitsNode.IsDefined()){
        if (limitsNode["low"].IsDefined()){
            attribs->m_lowerLimit = limitsNode["low"].as<double>();
        }
        if (limitsNode["high"].IsDefined()){
            attribs->m_upperLimit = limitsNode["high"].as<double>();
        }
        if (limitsNode["low"].IsDefined() && limitsNode["high"].IsDefined()){
            attribs->m_enableLimits = true;
        }
    }

    if(enableMotorNode.IsDefined()){
        attribs->m_enableMotor = enableFeedbackNode.as<bool>();
    }

    if(maxMotorImpulseNode.IsDefined()){
        attribs->m_maxMotorImpulse = maxMotorImpulseNode.as<double>();
    }

    if(stiffnessNode.IsDefined()){
        attribs->m_stiffness = stiffnessNode.as<double>();
    }

    if(equilibriumPointNode.IsDefined()){
        attribs->m_equilibriumPoint = equilibriumPointNode.as<double>();
    }
    else{
        attribs->m_equilibriumPoint = attribs->m_lowerLimit + (attribs->m_upperLimit - attribs->m_lowerLimit) / 2.0;
    }

    if(erpNode.IsDefined()){
        attribs->m_erp = erpNode.as<double>();
    }

    if(cfmNode.IsDefined()){
        attribs->m_cfm = cfmNode.as<double>();
    }

    if (ignoreInterCollisionNode.IsDefined()){
       attribs->m_ignoreInterCollision = ignoreInterCollisionNode.as<bool>();
    }

    if (enableFeedbackNode.IsDefined()){
        attribs->m_enableFeedback = enableFeedbackNode.as<bool>();
    }

    return true;
}

bool ADFLoader_1_0::loadSensorAttribs(YAML::Node *a_node, afSensorAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: SENSOR'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    bool result = true;
    // Declare all the yaml parameters that we want to look for
    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];
    YAML::Node parentNameNode = node["parent"];
    YAML::Node posNode = node["location"]["position"];
    YAML::Node rotNode = node["location"]["orientation"];
    YAML::Node publishFrequencyNode = node["publish frequency"];

    ADFUtils adfUtils;

    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getHierarchyAttribsFromNode(&node, &attribs->m_hierarchyAttribs);
    adfUtils.getKinematicAttribsFromNode(&node, &attribs->m_kinematicAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);

    return result;
}

bool ADFLoader_1_0::loadRayTracerSensorAttribs(YAML::Node *a_node, afRayTracerSensorAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: SENSOR'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    attribs->m_sensorType = afSensorType::RAYTRACER;

    bool result = true;
    // Declare all the yaml parameters that we want to look for
    YAML::Node parentNameNode = node["parent"];
    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];
    YAML::Node posNode = node["location"]["position"];
    YAML::Node rotNode = node["location"]["orientation"];
    YAML::Node publishFrequencyNode = node["publish frequency"];
    YAML::Node rangeNode = node["range"];
    YAML::Node visibleNode = node["visible"];
    YAML::Node visibleSizeNode = node["visible size"];
    YAML::Node arrayNode = node["array"];
    YAML::Node meshNode = node["mesh"];
    YAML::Node parametricNode = node["parametric"];

    loadSensorAttribs(&node, attribs);

    attribs->m_specificationType = afSensactorSpecificationType::INVALID;

    if(rangeNode.IsDefined()){
        attribs->m_range = rangeNode.as<double>();
    }

    if (visibleNode.IsDefined()){
        attribs->m_visible = visibleNode.as<bool>();
    }

    if (visibleSizeNode.IsDefined()){
        attribs->m_visibleSize = visibleSizeNode.as<double>();
    }

    if (arrayNode.IsDefined()){
        uint count = arrayNode.size();
        attribs->m_raysAttribs.resize(count);
        for (uint i = 0 ; i < count ; i++){
            YAML::Node offsetNode = arrayNode[i]["offset"];
            YAML::Node directionNode = arrayNode[i]["direction"];

            afVector3d localStart, localDir, localEnd;

            localStart = ADFUtils::positionFromNode(&offsetNode);
            localDir = ADFUtils::positionFromNode(&directionNode);
            localEnd = localStart + localDir * attribs->m_range;

            attribs->m_raysAttribs[i].m_range = attribs->m_range;
            attribs->m_raysAttribs[i].m_rayFromLocal = localStart;
            attribs->m_raysAttribs[i].m_direction = localDir;
            attribs->m_raysAttribs[i].m_rayToLocal = localEnd;
        }
        attribs->m_specificationType = afSensactorSpecificationType::ARRAY;
        result = true;
    }

    else if (meshNode.IsDefined()){
        // We are not going to load the mesh in the loader and this would restrict us in using a
        // specific mesh processing library, thus let the ambf_framework load the mesh.
        attribs->m_contourMeshFilepath = meshNode.as<string>();
        attribs->m_specificationType = afSensactorSpecificationType::MESH;
        result = true;
    }
    else if (parametricNode.IsDefined()){
        YAML::Node resolutionNode = parametricNode["resolution"];
        YAML::Node horSpanNode = parametricNode["horizontal angle"];
        YAML::Node verSpanNode = parametricNode["vertical angle"];
        YAML::Node startOffsetNode = parametricNode["start offset"];

        uint resolution = resolutionNode.as<uint>();
        double horizontal_span = horSpanNode.as<double>();
        double vertical_span = verSpanNode.as<double>();
        double start_offset = startOffsetNode.as<double>();

        if (resolution < 2){
            cerr << "ERROR! FOR SENSOR \"" << attribs->m_identificationAttribs.m_name << "\" RESOLUTION MUST BE GREATER THAN EQUAL TO 2. IGNORING! \n";
            return false;
        }

        double h_start = -horizontal_span / 2.0;
        double v_start = -vertical_span / 2.0;
        double h_step = horizontal_span / (resolution - 1);
        double v_step = vertical_span / (resolution - 1);
        attribs->m_raysAttribs.resize(resolution * resolution);

        // Choose an initial point facing the +ve x direction
        afVector3d nx(1, 0, 0);
        afTransform T_sINp = attribs->m_kinematicAttribs.m_location;
        afMatrix3d R_sINp = T_sINp.getRotation();
        for (uint i = 0 ; i < resolution ; i++){
            double h_angle = h_start + i * h_step;
            for (uint j = 0 ; j < resolution ; j++){
                double v_angle = v_start + j * v_step;

                afMatrix3d deltaR(0, v_angle, h_angle);
                afVector3d start, dir, end;

                start = T_sINp * (deltaR * (nx * start_offset));
                dir = R_sINp * deltaR * nx;
                dir.normalize();
                end = start + dir * attribs->m_range;

                uint sIdx = resolution * i + j;
                attribs->m_raysAttribs[sIdx].m_range = attribs->m_range;
                attribs->m_raysAttribs[sIdx].m_rayFromLocal = start;
                attribs->m_raysAttribs[sIdx].m_direction = dir;
                attribs->m_raysAttribs[sIdx].m_rayToLocal = end;
            }

        }
        result = true;
        attribs->m_specificationType = afSensactorSpecificationType::PARAMETRIC;
    }
    else{
        result = false;
    }

    return result;

}

bool ADFLoader_1_0::loadResistanceSensorAttribs(YAML::Node *a_node, afResistanceSensorAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: ACTUATOR'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    bool result = false;
        result = loadRayTracerSensorAttribs(a_node, attribs);

        attribs->m_sensorType = afSensorType::RESISTANCE;

        if (result){

            YAML::Node resistiveFrictionNode = node["friction"];
            YAML::Node contactAreaNode = node["contact area"];
            YAML::Node contactStiffnessNode = node["contact stiffness"];
            YAML::Node contactDampingNode = node["contact damping"];

            if (resistiveFrictionNode["static"].IsDefined()){
                attribs->m_staticContactFriction = resistiveFrictionNode["static"].as<double>();
            }

            if (resistiveFrictionNode["damping"].IsDefined()){
                attribs->m_staticContactDamping = resistiveFrictionNode["damping"].as<double>();
            }

            if (resistiveFrictionNode["dynamic"].IsDefined()){
                attribs->m_dynamicFriction = resistiveFrictionNode["dynamic"].as<double>();
            }

            if (resistiveFrictionNode["variable"].IsDefined()){
                attribs->m_useVariableCoeff = resistiveFrictionNode["variable"].as<bool>();
            }

            if (contactAreaNode.IsDefined()){
                attribs->m_contactArea = contactAreaNode.as<double>();
            }

            if (contactStiffnessNode.IsDefined()){
                attribs->m_contactNormalStiffness = contactStiffnessNode.as<double>();
            }

            if (contactDampingNode.IsDefined()){
                attribs->m_contactNormalDamping = contactDampingNode.as<double>();
            }
        }
        return result;
}

bool ADFLoader_1_0::loadActuatorAttribs(YAML::Node *a_node, afActuatorAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: ACTUATOR'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    bool result = true;
    // Declare all the yaml parameters that we want to look for
    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];
    YAML::Node parentNameNode = node["parent"];
    YAML::Node posNode = node["location"]["position"];
    YAML::Node rotNode = node["location"]["orientation"];
    YAML::Node publishFrequencyNode = node["publish frequency"];

    ADFUtils adfUtils;

    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getHierarchyAttribsFromNode(&node, &attribs->m_hierarchyAttribs);
    adfUtils.getKinematicAttribsFromNode(&node, &attribs->m_kinematicAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);

    return result;
}

bool ADFLoader_1_0::loadConstraintActuatorAttribs(YAML::Node *a_node, afConstraintActuatorAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: ACTUATOR'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    bool result = true;
    // Declare all the yaml parameters that we want to look for
    YAML::Node nameNode = node["name"];
    YAML::Node namespaceNode = node["namespace"];
    YAML::Node parentNameNode = node["parent"];
    YAML::Node posNode = node["location"]["position"];
    YAML::Node rotNode = node["location"]["orientation"];
    YAML::Node publishFrequencyNode = node["publish frequency"];
    YAML::Node visibleNode = node["visible"];
    YAML::Node visibleSizeNode = node["visible size"];
    YAML::Node maxImpulseNode = node["max impulse"];
    YAML::Node tauNode = node["tau"];

    loadActuatorAttribs(&node, attribs);

    if (visibleNode.IsDefined()){
        attribs->m_visible = visibleNode.as<bool>();
    }

    if (visibleSizeNode.IsDefined()){
        attribs->m_visibleSize = visibleSizeNode.as<bool>();
    }

    if (maxImpulseNode.IsDefined()){
        attribs->m_maxImpulse = maxImpulseNode.as<double>();
    }

    if (tauNode.IsDefined()){
        attribs->m_tau = tauNode.as<double>();
    }

    attribs->m_actuatorType = afActuatorType::CONSTRAINT;

    return result;
}

bool ADFLoader_1_0::loadVehicleAttribs(YAML::Node* a_node, afVehicleAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: VEHICLE'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    bool result = true;
    // Declare all the yaml parameters that we want to look for
    YAML::Node nameNode = node["name"];
    YAML::Node nameSpaceNode = node["namespace"];
    YAML::Node chassisNode = node["chassis"];
    YAML::Node meshPathHRNode = node["high resolution path"];
    YAML::Node wheelsNode = node["wheels"];

    ADFUtils adfUtils;

    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);
    adfUtils.getCommunicationAttribsFromNode(&node, &attribs->m_communicationAttribs);

    if (chassisNode.IsDefined()){
        attribs->m_chassisBodyName = chassisNode.as<string>();
    }

    if (meshPathHRNode.IsDefined()){
        attribs->m_wheelsVisualPath = meshPathHRNode.as<string>();
    }

    for (uint i = 0 ; i < wheelsNode.size() ; i++){
        afWheelAttributes wheelAttribs;
        YAML::Node wheelNode = wheelsNode[i];
        if (adfUtils.getWheelAttribsFromNode(&wheelNode, & wheelAttribs)){
            attribs->m_wheelAttribs.push_back( wheelAttribs);
        }
    }

    return result;
}


bool ADFLoader_1_0::loadInputDeviceAttributes(YAML::Node* a_node, afInputDeviceAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: PHYSICAL DEVICE'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    YAML::Node hardwareNameNode = node["hardware name"];
    YAML::Node hapticGainNode = node["haptic gain"];
    YAML::Node deadbandNode = node["deadband"];
    YAML::Node maxForceNode = node["max force"];
    YAML::Node maxJerkNode = node["max jerk"];
    YAML::Node workspaceScalingNode = node["workspace scaling"];
    YAML::Node orientationOffsetNode = node["orientation offset"];
    YAML::Node buttonMappingNode = node["button mapping"];
    YAML::Node visibleNode = node["visible"];
    YAML::Node visibleSizeNode = node["visible size"];
    YAML::Node visibleColorNode = node["visible color"];

    // For the simulated gripper, the user can specify a model config to load.
    // We shall load this file as a proxy for Physical Input device in the simulation.
    // We shall get the root link of this model (baselink) and set Cartesian Position
    // control on this body.

    // Further, the user can sepcify a root link for the model config file. If this
    // is defined we shall infact use the specific link which can be different from
    // the bodies base link.

    // A second use case arises, in which the user doesnt want to provide a config file
    // but wants to bind the physical input device to an existing model in the simulation.
    // In this case, the user should specify just the root link and we shall try to find a
    // body in simulation matching that name. Once succesful we shall then be able to control
    // that link/body in Position control mode and control all the joints lower in hierarchy.

    if (hardwareNameNode.IsDefined()){
        attribs->m_hardwareName = hardwareNameNode.as<string>();
    }
    else{
        cerr << "ERROR: PHYSICAL DEVICES HARDWARE NAME NOT DEFINED, IGNORING \n";
        return 0;
    }

    if (workspaceScalingNode.IsDefined()){
        attribs->m_workspaceScale = workspaceScalingNode.as<double>();
    }

    if (hapticGainNode.IsDefined()){
        attribs->m_controllerAttribs.P_lin = hapticGainNode["linear"].as<double>();
        attribs->m_controllerAttribs.P_ang = hapticGainNode["angular"].as<double>();
        attribs->m_controllerAttribs.I_lin = 0;
        attribs->m_controllerAttribs.I_ang = 0;
        attribs->m_controllerAttribs.D_lin = 0;
        attribs->m_controllerAttribs.D_ang = 0;
    }

    if (deadbandNode.IsDefined()){
        attribs->m_deadBand = deadbandNode.as<double>();
    }

    if (maxForceNode.IsDefined()){
        attribs->m_maxForce = maxForceNode.as<double>();
    }

    if (maxJerkNode.IsDefined()){
        attribs->m_maxJerk = maxJerkNode.as<double>();
    }

    if (orientationOffsetNode.IsDefined()){
        afMatrix3d rot = ADFUtils::rotationFromNode(&orientationOffsetNode);
        attribs->m_orientationOffset.setRotation(rot);
    }

    if (buttonMappingNode.IsDefined()){
        if (buttonMappingNode["a1"].IsDefined()){
            attribs->m_buttons.A1 = buttonMappingNode["a1"].as<int>();
        }
        if (buttonMappingNode["a2"].IsDefined()){
            attribs->m_buttons.A2 = buttonMappingNode["a2"].as<int>();
        }
        if (buttonMappingNode["g1"].IsDefined()){
            attribs->m_buttons.G1 = buttonMappingNode["g1"].as<int>();
        }
        if (buttonMappingNode["next mode"].IsDefined()){
            attribs->m_buttons.NEXT_MODE = buttonMappingNode["next mode"].as<int>();
        }
        if (buttonMappingNode["prev mode"].IsDefined()){
            attribs->m_buttons.PREV_MODE = buttonMappingNode["prev mode"].as<int>();
        }
    }

    if (visibleNode.IsDefined()){
        attribs->m_visible = visibleNode.as<bool>();
    }

    if (visibleSizeNode.IsDefined()){
        attribs->m_visibleSize = visibleSizeNode.as<double>();
    }

    return true;
}

bool ADFLoader_1_0::loadSimulatedDeviceAttributes(YAML::Node *a_node, afSimulatedDeviceAttribs *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: PHYSICAL DEVICE'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    YAML::Node enableJointControlNode = node["enable joint control"];
    YAML::Node simulatedModelNode = node["simulated multibody"];
    YAML::Node rootLinkNode = node["root link"];
    YAML::Node locationNode = node["location"];
    YAML::Node controllerGainNode = node["controller gain"];

    // For the simulated gripper, the user can specify a model config to load.
    // We shall load this file as a proxy for Physical Input device in the simulation.
    // We shall get the root link of this model (baselink) and set Cartesian Position
    // control on this body.

    // Further, the user can sepcify a root link for the model config file. If this
    // is defined we shall infact use the specific link which can be different from
    // the bodies base link.

    // A second use case arises, in which the user doesnt want to provide a config file
    // but wants to bind the physical input device to an existing model in the simulation.
    // In this case, the user should specify just the root link and we shall try to find a
    // body in simulation matching that name. Once succesful we shall then be able to control
    // that link/body in Position control mode and control all the joints lower in hierarchy.

    ADFUtils adfUtils;

    if (locationNode.IsDefined()){
        adfUtils.getKinematicAttribsFromNode(&node, &attribs->m_kinematicAttribs);
        attribs->m_overrideLocation = true;
    }

    // set the control gains fields as controller fields.
    if (controllerGainNode.IsDefined()){
        node["controller"] = controllerGainNode;
        adfUtils.getCartControllerAttribsFromNode(&node, &attribs->m_controllerAttribs);
        attribs->m_overrideController = true;
    }

    if (simulatedModelNode.IsDefined()){
        afPath sdeFilepath = simulatedModelNode.as<string>();
        sdeFilepath.resolvePath(attribs->m_filePath.parent_path());

        if (loadModelAttribs(sdeFilepath.c_str(), &attribs->m_modelAttribs)){
            attribs->m_sdeDefined = true;
        }
        else{
            attribs->m_sdeDefined = false;
        }

    }
    else{
        attribs->m_sdeDefined = false;
    }


    if (rootLinkNode.IsDefined()){
        attribs->m_rootLinkName = rootLinkNode.as<string>();
        attribs->m_rootLinkDefined = true;
    }
    else{
        attribs->m_rootLinkDefined = false;
    }

    if (enableJointControlNode.IsDefined()){
        attribs->m_enableJointControl = enableJointControlNode.as<bool>();
    }

    return true;
}


bool ADFLoader_1_0::loadAllTeleRoboticUnitsAttribs(string a_filepath, afAllTeleRoboticUnitsAttributes *attribs){
    attribs->m_filePath = afPath(a_filepath);
    YAML::Node node = YAML::LoadFile(a_filepath);
    return loadAllTeleRoboticUnitsAttribs(&node, attribs);
}


bool ADFLoader_1_0::loadAllTeleRoboticUnitsAttribs(YAML::Node *a_node, afAllTeleRoboticUnitsAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: ALL INPUT DEVICES'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    YAML::Node inputDevicesNode = node["input devices"];

    for (int i = 0 ; i < inputDevicesNode.size() ; i++){
        afTeleRoboticUnitAttributes tuAttribs;
        std::string devName = inputDevicesNode[i].as<std::string>();
        YAML::Node tuNode = node[devName];
        bool results[2] = {false, false};
        if (loadInputDeviceAttributes(&tuNode, &tuAttribs.m_iidAttribs)){
            results[0] = true;
        }
        tuAttribs.m_sdeAttribs.m_filePath = attribs->m_filePath;
        if (loadSimulatedDeviceAttributes(&tuNode, &tuAttribs.m_sdeAttribs)){
            results[1] = true;
        }

        YAML::Node pairCamerasNode = tuNode["pair cameras"];

        if(pairCamerasNode.IsDefined() && results[0] == true && results[1] == true){
            for(int i = 0 ; i < pairCamerasNode.size() ; i++){
                string camName = pairCamerasNode[i].as<string>();
                tuAttribs.m_pairedCamerasNames.push_back(camName);
            }
        }

        if (results[0] == true && results[1] == true){
            attribs->m_teleRoboticUnitsAttribs.push_back(tuAttribs);
        }
        else{
            // THROW SOME WARNING
        }
    }

    return true;
}


bool ADFLoader_1_0::loadModelAttribs(string a_filepath, afModelAttributes *attribs){
    attribs->m_filePath = afPath(a_filepath);
    try{
        YAML::Node node = YAML::LoadFile(a_filepath);
        return loadModelAttribs(&node, attribs);
    }
    catch(exception e){
        std::cerr << e.what();
        return false;
    }
}


bool ADFLoader_1_0::loadModelAttribs(YAML::Node *a_node, afModelAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: MODEL'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    // Declare all the yaml parameters that we want to look for
    YAML::Node meshPathHRNode = node["high resolution path"];
    YAML::Node meshPathLRNode = node["low resolution path"];
    YAML::Node nameSpaceNode = node["namespace"];
    YAML::Node rigidBodiesNode = node["bodies"];
    YAML::Node softBodiesNode = node["soft bodies"];
    YAML::Node vehiclesNode = node["vehicles"];
    YAML::Node jointsNode = node["joints"];
    YAML::Node sensorsNode = node["sensors"];
    YAML::Node actuatorsNode = node["actuators"];
    YAML::Node jointERPNode = node["joint erp"];
    YAML::Node jointCFMNode = node["joint cfm"];
    YAML::Node ignoreInterCollisionNode = node["ignore inter-collision"];

    bool valid = true;

    attribs->m_identificationAttribs.m_name = attribs->m_filePath.filename().c_str();

    if(meshPathHRNode.IsDefined()){
        attribs->m_visualMeshesPath = meshPathHRNode.as<string>();
    }

    if(meshPathLRNode.IsDefined()){
        attribs->m_collisionMeshesPath = meshPathLRNode.as<string>();
    }

    if (nameSpaceNode.IsDefined()){
        attribs->m_identificationAttribs.m_namespace = nameSpaceNode.as<string>();
    }

    // Loading Rigid Bodies
    for (size_t i = 0; i < rigidBodiesNode.size(); ++i) {
        afRigidBodyAttributes rbAttribs;
        string identifier = rigidBodiesNode[i].as<string>();
        YAML::Node rbNode = node[identifier];
        if (loadRigidBodyAttribs(&rbNode, &rbAttribs)){
            rbAttribs.m_identifier = identifier;
            rbAttribs.m_visualAttribs.m_meshFilepath.resolvePath(attribs->m_visualMeshesPath);
            rbAttribs.m_collisionAttribs.m_meshFilepath.resolvePath(attribs->m_collisionMeshesPath);
            attribs->m_rigidBodyAttribs.push_back(rbAttribs);
        }
    }

    // Loading Soft Bodies
    for (size_t i = 0; i < softBodiesNode.size(); ++i) {
        afSoftBodyAttributes sbAttribs;
        string identifier = softBodiesNode[i].as<string>();
        YAML::Node sbNode = node[identifier];
        if (loadSoftBodyAttribs(&sbNode, &sbAttribs)){
            sbAttribs.m_identifier = identifier;
            sbAttribs.m_visualAttribs.m_meshFilepath.resolvePath(attribs->m_visualMeshesPath);
            sbAttribs.m_collisionAttribs.m_meshFilepath.resolvePath(attribs->m_collisionMeshesPath);
            attribs->m_softBodyAttribs.push_back(sbAttribs);
        }
    }

    // Loading Sensors
    for (size_t i = 0; i < sensorsNode.size(); ++i) {
        string identifier = sensorsNode[i].as<string>();
        YAML::Node senNode = node[identifier];
        // Check which type of sensor is this so we can cast appropriately beforehand
        if (senNode["type"].IsDefined()){
            afSensorType senType = ADFUtils::getSensorTypeFromString(senNode["type"].as<string>());
            switch (senType) {
            case afSensorType::RAYTRACER:{
                afRayTracerSensorAttributes* senAttribs = new afRayTracerSensorAttributes();
                if(loadRayTracerSensorAttribs(&senNode, senAttribs)){
                    senAttribs->m_identifier = identifier;
                    attribs->m_sensorAttribs.push_back(senAttribs);
                }
                break;
            }
            case afSensorType::RESISTANCE:{
                afResistanceSensorAttributes* senAttribs = new afResistanceSensorAttributes();
                if (loadResistanceSensorAttribs(&senNode, senAttribs)){
                    senAttribs->m_identifier = identifier;
                    attribs->m_sensorAttribs.push_back(senAttribs);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    /// Loading Actuators
    size_t totalActuators = actuatorsNode.size();
    for (size_t i = 0; i < totalActuators; ++i) {
        string identifier = actuatorsNode[i].as<string>();
        YAML::Node actNode = node[identifier];
        // Check which type of sensor is this so we can cast appropriately beforehand
        if (actNode["type"].IsDefined()){
            afActuatorType actType = ADFUtils::getActuatorTypeFromString(actNode["type"].as<string>());
            // Check if this is a constraint sensor
            switch (actType) {
            case afActuatorType::CONSTRAINT:{
                afConstraintActuatorAttributes* acAttribs = new afConstraintActuatorAttributes();
                if (loadConstraintActuatorAttribs(&actNode, acAttribs)){
                    acAttribs->m_identifier = identifier;
                    attribs->m_actuatorAttribs.push_back(acAttribs);
                }
                break;
            }
            default:
                break;
            }
        }
    }

//    if (jointERPNode.IsDefined()){
//        m_jointERP = jointERPNode.as<double>();
//    }
//    if (jointCFMNode.IsDefined()){
//        m_jointCFM = jointCFMNode.as<double>();
//    }

    // Loading Joints
    for (size_t i = 0; i < jointsNode.size(); ++i) {
        afJointAttributes jntAttribs;
        string identifier = jointsNode[i].as<string>();
        YAML::Node jntNode = node[identifier];
        if (loadJointAttribs(&jntNode, &jntAttribs)){
            jntAttribs.m_identifier = identifier;
            attribs->m_jointAttribs.push_back(jntAttribs);
        }
    }


    for (size_t i = 0; i < vehiclesNode.size(); ++i) {
        afVehicleAttributes vehAttribs;
        string identifier = vehiclesNode[i].as<string>();
        YAML::Node veh_node = node[identifier];
        if (loadVehicleAttribs(&veh_node, &vehAttribs)){
            vehAttribs.m_identifier = identifier;
            attribs->m_vehicleAttribs.push_back(vehAttribs);
        }
    }

    // This flag would ignore collision for all the multibodies in the scene
    if (ignoreInterCollisionNode.IsDefined()){
        attribs->m_ignoreInterCollision = ignoreInterCollisionNode.as<bool>();
    }

    return valid;
}


bool ADFLoader_1_0::loadWorldAttribs(string a_filepath, afWorldAttributes *attribs){
    attribs->m_filePath = afPath(a_filepath);
    try{
    YAML::Node node = YAML::LoadFile(a_filepath);
    return loadWorldAttribs(&node, attribs);
    }
    catch(exception e){
        cerr << e.what() ;
        return false;
    }
}


bool ADFLoader_1_0::loadWorldAttribs(YAML::Node *a_node, afWorldAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: WORLD'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    YAML::Node namespaceNode = node["namespace"];
    YAML::Node enclosureDataNode = node["enclosure"];
    YAML::Node lightsNode = node["lights"];
    YAML::Node camerasNode = node["cameras"];
    YAML::Node environmentNode = node["environment"];
    YAML::Node skyBoxNode = node["skybox"];
    YAML::Node maxIterationsNode = node["max iterations"];
    YAML::Node gravityNode = node["gravity"];
    YAML::Node shadersNode = node["shaders"];

    // Set the world name
    node["name"] = "World";

    ADFUtils adfUtils;

    adfUtils.getIdentificationAttribsFromNode(&node, &attribs->m_identificationAttribs);

    if(maxIterationsNode.IsDefined()){
        attribs->m_maxIterations = maxIterationsNode.as<uint>();
    }

    if (gravityNode.IsDefined()){
        attribs->m_gravity = ADFUtils::positionFromNode(&gravityNode);
    }

    if (enclosureDataNode.IsDefined()){
        attribs->m_enclosure.m_use = true;
        attribs->m_enclosure.m_length = enclosureDataNode["length"].as<double>();
        attribs->m_enclosure.m_width =  enclosureDataNode["width"].as<double>();
        attribs->m_enclosure.m_height = enclosureDataNode["height"].as<double>();
    }

    if (environmentNode.IsDefined()){
        afPath environmentFilepath = environmentNode.as<string>();
        environmentFilepath.resolvePath(attribs->m_filePath.parent_path().c_str());
        if (loadModelAttribs(environmentFilepath.c_str(), &attribs->m_environmentModel.m_modelAttribs)){
            attribs->m_environmentModel.m_use = true;
            attribs->m_enclosure.m_use = false;
        }

    }


    afPath localPath;

    if (skyBoxNode.IsDefined()){
        localPath = skyBoxNode["path"].as<string>();

        if (skyBoxNode["right"].IsDefined() &&
                skyBoxNode["left"].IsDefined() &&
                skyBoxNode["top"].IsDefined() &&
                skyBoxNode["bottom"].IsDefined() &&
                skyBoxNode["front"].IsDefined() &&
                skyBoxNode["back"].IsDefined()
                )
        {

            attribs->m_skyBoxAttribs.m_leftImageFilepath = localPath / skyBoxNode["left"].as<string>();
            attribs->m_skyBoxAttribs.m_rightImageFilepath = localPath / skyBoxNode["right"].as<string>();
            attribs->m_skyBoxAttribs.m_topImageFilepath = localPath / skyBoxNode["top"].as<string>();
            attribs->m_skyBoxAttribs.m_bottomImageFilepath = localPath / skyBoxNode["bottom"].as<string>();
            attribs->m_skyBoxAttribs.m_frontImageFilepath = localPath / skyBoxNode["front"].as<string>();
            attribs->m_skyBoxAttribs.m_backImageFilepath = localPath / skyBoxNode["back"].as<string>();

            adfUtils.getShaderAttribsFromNode(&skyBoxNode, &attribs->m_skyBoxAttribs.m_shaderAttribs);

            attribs->m_skyBoxAttribs.m_use = true;
        }
    }
    else{
        attribs->m_skyBoxAttribs.m_use = false;
    }

    if (lightsNode.IsDefined()){
        for (size_t idx = 0 ; idx < lightsNode.size(); idx++){
            string identifier = lightsNode[idx].as<string>();
            afLightAttributes lightAttribs;
            YAML::Node lightNode = node[identifier];
            if (loadLightAttribs(&lightNode, &lightAttribs)){
                lightAttribs.m_identifier = identifier;
                attribs->m_lightAttribs.push_back(lightAttribs);
            }
        }
    }

    if (camerasNode.IsDefined()){
        for (size_t idx = 0 ; idx < camerasNode.size(); idx++){
            string identifier = camerasNode[idx].as<string>();
            afCameraAttributes cameraAttribs;
            YAML::Node cameraNode = node[identifier];
            if (loadCameraAttribs(&cameraNode, &cameraAttribs)){
                cameraAttribs.m_identifier = identifier;
                attribs->m_cameraAttribs.push_back(cameraAttribs);
            }
        }
    }

    adfUtils.getShaderAttribsFromNode(&node, &attribs->m_shaderAttribs);

    return true;
}


bool ADFLoader_1_0::loadLaunchFileAttribs(string a_filepath, afLaunchAttributes *attribs){
    attribs->m_filePath = afPath(a_filepath);
    try{
        YAML::Node node = YAML::LoadFile(a_filepath);
        return loadLaunchFileAttribs(&node, attribs);
    }
    catch(exception e){
        cerr << e.what();
        return false;
    }
}

bool ADFLoader_1_0::loadLaunchFileAttribs(YAML::Node *a_node, afLaunchAttributes *attribs)
{
    YAML::Node& node = *a_node;
    if (node.IsNull()){
        cerr << "ERROR: LAUNCH FILE'S YAML CONFIG DATA IS NULL\n";
        return 0;
    }

    //Declare all the YAML Params that we want to look for
    YAML::Node worldFilepathNode = node["world config"];
    YAML::Node colorFilepathNode = node["color config"];
    YAML::Node inputDevicesFilepathNode = node["input devices config"];
    YAML::Node modelFilepathsNode = node["multibody configs"];


    if(worldFilepathNode.IsDefined()){
        attribs->m_worldFilepath = worldFilepathNode.as<string>();
    }
    else{
        cerr << "ERROR! WORLD CONFIG NOT DEFINED \n";
        return 0;
    }

    if(inputDevicesFilepathNode.IsDefined()){
        attribs->m_inputDevicesFilepath = inputDevicesFilepathNode.as<string>();
    }
    else{
        cerr << "ERROR! INPUT DEVICES CONFIG NOT DEFINED \n";
        return 0;
    }

    if(colorFilepathNode.IsDefined()){
        attribs->m_colorFilepath  = colorFilepathNode.as<string>();
    }
    else{
        return 0;
    }

    if (modelFilepathsNode.IsDefined()){
        for (size_t i = 0 ; i < modelFilepathsNode.size() ; i++){
            attribs->m_modelFilepaths.push_back(modelFilepathsNode[i].as<string>());
        }
    }
    else{
        cerr << "PATH AND MODEL CONFIG NOT DEFINED \n";
        return 0;
    }

    return 1;
}