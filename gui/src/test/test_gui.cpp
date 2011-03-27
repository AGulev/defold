#include <map>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <dlib/hash.h>
#include <dlib/message.h>
#include <script/script.h>
#include "../gui.h"
#include "../gui_private.h"
#include "test_gui_ddf.h"

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
}

extern char BUG352_LUA[];
extern uint32_t BUG352_LUA_SIZE;


/*
 * Basic
 *  - Create scene
 *  - Create nodes
 *  - Stress tests
 *
 * self table
 *
 * reload script
 *
 * lua script basics
 *  - New/Delete node
 *
 * "Namespaces"
 *
 * Animation
 *
 * Render
 *
 */

#define MAX_NODES 64U
#define MAX_ANIMATIONS 32U

class dmGuiTest : public ::testing::Test
{
protected:
    dmGui::HContext m_Context;
    dmGui::HScene m_Scene;
    dmMessage::HSocket m_Socket;
    dmGui::HScript m_Script;

    virtual void SetUp()
    {
        dmMessage::NewSocket("test_m_Socket", &m_Socket);
        dmGui::NewContextParams context_params;
        context_params.m_Socket = m_Socket;

        m_Context = dmGui::NewContext(&context_params);
        dmGui::RegisterDDFType(dmTestGuiDDF::AMessage::m_DDFDescriptor);
        dmGui::NewSceneParams params;
        params.m_MaxNodes = MAX_NODES;
        params.m_MaxAnimations = MAX_ANIMATIONS;
        m_Scene = dmGui::NewScene(m_Context, &params);
        m_Script = dmGui::NewScript(m_Context);
        dmGui::SetSceneScript(m_Scene, m_Script);
    }

    virtual void TearDown()
    {
        dmGui::DeleteScript(m_Script);
        dmGui::DeleteScene(m_Scene);
        dmGui::DeleteContext(m_Context);
        dmMessage::DeleteSocket(m_Socket);
    }
};

TEST_F(dmGuiTest, Basic)
{
    for (uint32_t i = 0; i < MAX_NODES; ++i)
    {
        dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(5,5,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
        ASSERT_NE((dmGui::HNode) 0, node);
    }

    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(5,5,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    ASSERT_EQ((dmGui::HNode) 0, node);
}

TEST_F(dmGuiTest, Name)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(5,5,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    ASSERT_NE((dmGui::HNode) 0, node);

    dmGui::HNode get_node = dmGui::GetNodeByName(m_Scene, "my_node");
    ASSERT_EQ((dmGui::HNode) 0, get_node);

    dmGui::SetNodeName(m_Scene, node, "my_node");
    get_node = dmGui::GetNodeByName(m_Scene, "my_node");
    ASSERT_EQ(node, get_node);
}

TEST_F(dmGuiTest, TextureFont)
{
    int t1, t2;
    int f1, f2;

    dmGui::AddTexture(m_Scene, "t1", (void*) &t1);
    dmGui::AddTexture(m_Scene, "t2", (void*) &t2);
    dmGui::AddFont(m_Scene, "f1", &f1);
    dmGui::AddFont(m_Scene, "f2", &f2);

    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(5,5,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    ASSERT_NE((dmGui::HNode) 0, node);

    dmGui::Result r;

    // Texture
    r = dmGui::SetNodeTexture(m_Scene, node, "foo");
    ASSERT_EQ(r, dmGui::RESULT_RESOURCE_NOT_FOUND);

    r = dmGui::SetNodeTexture(m_Scene, node, "f1");
    ASSERT_EQ(r, dmGui::RESULT_RESOURCE_NOT_FOUND);

    r = dmGui::SetNodeTexture(m_Scene, node, "t1");
    ASSERT_EQ(r, dmGui::RESULT_OK);

    r = dmGui::SetNodeTexture(m_Scene, node, "t2");
    ASSERT_EQ(r, dmGui::RESULT_OK);

    dmGui::AddTexture(m_Scene, "t2", &t1);
    ASSERT_EQ(&t1, m_Scene->m_Nodes[node & 0xffff].m_Node.m_Texture);

    dmGui::RemoveTexture(m_Scene, "t2");
    ASSERT_EQ((void*)0, m_Scene->m_Nodes[node & 0xffff].m_Node.m_Texture);

    r = dmGui::SetNodeTexture(m_Scene, node, "t2");
    ASSERT_EQ(r, dmGui::RESULT_RESOURCE_NOT_FOUND);

    dmGui::ClearTextures(m_Scene);
    r = dmGui::SetNodeTexture(m_Scene, node, "t1");
    ASSERT_EQ(r, dmGui::RESULT_RESOURCE_NOT_FOUND);

    // Font
    r = dmGui::SetNodeFont(m_Scene, node, "foo");
    ASSERT_EQ(r, dmGui::RESULT_RESOURCE_NOT_FOUND);

    r = dmGui::SetNodeFont(m_Scene, node, "t1");
    ASSERT_EQ(r, dmGui::RESULT_RESOURCE_NOT_FOUND);

    r = dmGui::SetNodeFont(m_Scene, node, "f1");
    ASSERT_EQ(r, dmGui::RESULT_OK);

    r = dmGui::SetNodeFont(m_Scene, node, "f2");
    ASSERT_EQ(r, dmGui::RESULT_OK);

    dmGui::AddFont(m_Scene, "f2", &f1);
    ASSERT_EQ(&f1, m_Scene->m_Nodes[node & 0xffff].m_Node.m_Font);

    dmGui::RemoveFont(m_Scene, "f2");
    ASSERT_EQ((void*)0, m_Scene->m_Nodes[node & 0xffff].m_Node.m_Font);

    dmGui::ClearFonts(m_Scene);
    r = dmGui::SetNodeFont(m_Scene, node, "f1");
    ASSERT_EQ(r, dmGui::RESULT_RESOURCE_NOT_FOUND);

    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, NewDeleteNode)
{
    std::map<dmGui::HNode, float> node_to_pos;

    for (uint32_t i = 0; i < MAX_NODES; ++i)
    {
        dmGui::HNode node = dmGui::NewNode(m_Scene, Point3((float) i, 0, 0), Vector3(0, 0 ,0), dmGui::NODE_TYPE_BOX);
        ASSERT_NE((dmGui::HNode) 0, node);
        node_to_pos[node] = (float) i;
    }

    for (uint32_t i = 0; i < 1000; ++i)
    {
        ASSERT_EQ(MAX_NODES, node_to_pos.size());

        std::map<dmGui::HNode, float>::iterator iter;
        for (iter = node_to_pos.begin(); iter != node_to_pos.end(); ++iter)
        {
            dmGui::HNode node = iter->first;
            ASSERT_EQ(iter->second, dmGui::GetNodePosition(m_Scene, node).getX());
        }
        int index = rand() % MAX_NODES;
        iter = node_to_pos.begin();
        for (int j = 0; j < index; ++j)
            ++iter;
        dmGui::HNode node_to_remove = iter->first;
        node_to_pos.erase(iter);
        dmGui::DeleteNode(m_Scene, node_to_remove);

        dmGui::HNode new_node = dmGui::NewNode(m_Scene, Point3((float) i, 0, 0), Vector3(0, 0 ,0), dmGui::NODE_TYPE_BOX);
        ASSERT_NE((dmGui::HNode) 0, new_node);
        node_to_pos[new_node] = (float) i;
    }
}

TEST_F(dmGuiTest, AnimateNode)
{
    for (uint32_t i = 0; i < MAX_ANIMATIONS + 1; ++i)
    {
        dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
        // NOTE: We need to add 0.001f or order to ensure that the delay will take exactly 30 frames
        dmGui::AnimateNode(m_Scene, node, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 1.0f, 0.5f + 0.001f, 0, 0, 0);

        ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

        // Delay
        for (int i = 0; i < 30; ++i)
            dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);

        ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

        // Animation
        for (int i = 0; i < 60; ++i)
        {
            dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        }

        ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 1.0f, 0.001f);
        dmGui::DeleteNode(m_Scene, node);
    }
}

TEST_F(dmGuiTest, AnimateNode2)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::AnimateNode(m_Scene, node, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 1.1f, 0, 0, 0, 0);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

    // Animation
    for (int i = 0; i < 200; ++i)
    {
        dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    }

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 1.0f, 0.001f);
    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, AnimateNodeDelayUnderFlow)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::AnimateNode(m_Scene, node, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 2.0f / 60.0f, 1.0f / 60.0f, 0, 0, 0);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

    dmGui::UpdateScene(m_Scene, 0.5f * (1.0f / 60.0f));
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

    dmGui::UpdateScene(m_Scene, 1.0f * (1.0f / 60.0f));
    // With underflow compensation and dt: (0.5 / 60.) + dt = 1.5 / 60
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.75f, 0.001f);

    dmGui::UpdateScene(m_Scene, 1.0f * (1.0f / 60.0f));
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 1.0f, 0.001f);

    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, AnimateNodeDelete)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::AnimateNode(m_Scene, node, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 1.1f, 0, 0, 0, 0);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);
    dmGui::HNode node2 = 0;

    // Animation
    for (int i = 0; i < 60; ++i)
    {
        if (i == 30)
        {
            dmGui::DeleteNode(m_Scene, node);
            node2 = dmGui::NewNode(m_Scene, Point3(2,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
        }

        dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    }

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node2).getX(), 2.0f, 0.001f);
    dmGui::DeleteNode(m_Scene, node2);
}

uint32_t MyAnimationCompleteCount = 0;
void MyAnimationComplete(dmGui::HScene scene,
                         dmGui::HNode node,
                         void* userdata1,
                         void* userdata2)
{
    MyAnimationCompleteCount++;
    dmGui::AnimateNode(scene, node, dmGui::PROPERTY_POSITION, Vector4(2,0,0,0), dmGui::EASING_NONE, 1.0f, 0, 0, 0, 0);
}

TEST_F(dmGuiTest, AnimateComplete)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::AnimateNode(m_Scene, node, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 1.0f, 0, &MyAnimationComplete, (void*) node, 0);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

    // Animation
    for (int i = 0; i < 60; ++i)
    {
        dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    }
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 1.0f, 0.001f);

    // Animation
    for (int i = 0; i < 60; ++i)
    {
        dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    }
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 2.0f, 0.001f);

    dmGui::DeleteNode(m_Scene, node);
}

void MyPingPongComplete2(dmGui::HScene scene,
                         dmGui::HNode node,
                         void* userdata1,
                         void* userdata2);

uint32_t PingPongCount = 0;
void MyPingPongComplete1(dmGui::HScene scene,
                        dmGui::HNode node,
                        void* userdata1,
                        void* userdata2)
{
    ++PingPongCount;
    dmGui::AnimateNode(m_Scene, node, dmGui::PROPERTY_POSITION, Vector4(0,0,0,0), dmGui::EASING_NONE, 1.0f, 0, &MyPingPongComplete2, (void*) node, 0);
}

void MyPingPongComplete2(dmGui::HScene scene,
                         dmGui::HNode node,
                         void* userdata1,
                         void* userdata2)
{
    ++PingPongCount;
    dmGui::AnimateNode(scene, node, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 1.0f, 0, &MyPingPongComplete1, (void*) node, 0);
}

TEST_F(dmGuiTest, PingPong)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::AnimateNode(m_Scene, node, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 1.0f, 0, &MyPingPongComplete1, (void*) node, 0);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

    for (int j = 0; j < 10; ++j)
    {
        // Animation
        for (int i = 0; i < 60; ++i)
        {
            dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        }
    }

    ASSERT_EQ(10U, PingPongCount);
    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, ScriptAnimate)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function init(self)\n"
                    // NOTE: We need to add 0.001f or order to ensure that the delay will take exactly 30 frames
                    "gui.animate(gui.get_node(\"n\"), gui.POSITION, vmath.vector4(1,0,0,0), gui.EASING_NONE, 1, 0.5 + 0.001)\n"
                    "end\n"
                    "function update(self)\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

    // Delay
    for (int i = 0; i < 30; ++i)
    {
        r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        ASSERT_EQ(dmGui::RESULT_OK, r);
    }

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);

    // Animation
    for (int i = 0; i < 60; ++i)
    {
        r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        ASSERT_EQ(dmGui::RESULT_OK, r);
    }

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 1.0f, 0.001f);

    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, ScriptAnimateComplete)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function cb(node)\n"
                    "gui.animate(node, gui.POSITION, vmath.vector4(2,0,0,0), gui.EASING_NONE, 0.5, 0)\n"
                    "end\n;"
                    "function init(self)\n"
                    "gui.animate(gui.get_node(\"n\"), gui.POSITION, vmath.vector4(1,0,0,0), gui.EASING_NONE, 1, 0, cb)\n"
                    "end\n"
                    "function update(self)\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 0.0f, 0.001f);
    // Animation
    for (int i = 0; i < 60; ++i)
    {
        r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        ASSERT_EQ(dmGui::RESULT_OK, r);
    }
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 1.0f, 0.001f);

    // Animation
    for (int i = 0; i < 30; ++i)
    {
        r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        ASSERT_EQ(dmGui::RESULT_OK, r);
    }
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node).getX(), 2.0f, 0.001f);

    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, ScriptAnimateCompleteDelete)
{
    dmGui::HNode node1 = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::HNode node2 = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node1, "n1");
    dmGui::SetNodeName(m_Scene, node2, "n2");
    const char* s = "function cb(node)\n"
                    "gui.delete_node(node)\n"
                    "end\n;"
                    "function init(self)\n"
                    "gui.animate(gui.get_node(\"n1\"), gui.POSITION, vmath.vector4(1,0,0,0), gui.EASING_NONE, 1, 0, cb)\n"
                    "gui.animate(gui.get_node(\"n2\"), gui.POSITION, vmath.vector4(1,0,0,0), gui.EASING_NONE, 1, 0, cb)\n"
                    "end\n"
                    "function update(self)\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    uint32_t node_count = dmGui::GetNodeCount(m_Scene);
    ASSERT_EQ(2U, node_count);

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node1).getX(), 0.0f, 0.001f);
    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node2).getX(), 0.0f, 0.001f);
    // Animation
    for (int i = 0; i < 60; ++i)
    {
        r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        ASSERT_EQ(dmGui::RESULT_OK, r);
    }

    node_count = dmGui::GetNodeCount(m_Scene);
    ASSERT_EQ(0U, node_count);
}

TEST_F(dmGuiTest, ScriptOutOfNodes)
{
    const char* s = "function init(self)\n"
                    "    for i=1,10000 do\n"
                    "       gui.new_box_node({0,0,0}, {1,1,1})\n"
                    "    end\n"
                    "end\n"
                    "function update(self)\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_SCRIPT_ERROR, r);
}

TEST_F(dmGuiTest, ScriptGetNode)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function update(self) local n = gui.get_node(\"n\")\n print(n)\n end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, ScriptGetMissingNode)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function update(self) local n = gui.get_node(\"x\")\n print(n)\n end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_SCRIPT_ERROR, r);

    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, ScriptGetDeletedNode)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function update(self) local n = gui.get_node(\"n\")\n print(n)\n end";
    dmGui::DeleteNode(m_Scene, node);

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_SCRIPT_ERROR, r);
}

TEST_F(dmGuiTest, ScriptEqNode)
{
    dmGui::HNode node1 = dmGui::NewNode(m_Scene, Point3(1,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::HNode node2 = dmGui::NewNode(m_Scene, Point3(2,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node1, "n");
    dmGui::SetNodeName(m_Scene, node2, "m");

    const char* s = "function update(self)\n"
                    "local n1 = gui.get_node(\"n\")\n "
                    "local n2 = gui.get_node(\"n\")\n "
                    "local m = gui.get_node(\"m\")\n "
                    "assert(n1 == n2)\n"
                    "assert(m ~= n1)\n"
                    "assert(m ~= n2)\n"
                    "assert(m ~= 1)\n"
                    "assert(1 ~= m)\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::DeleteNode(m_Scene, node1);
    dmGui::DeleteNode(m_Scene, node2);
}

TEST_F(dmGuiTest, ScriptNewNode)
{
    const char* s = "function init(self)\n"
                    "    self.n1 = gui.new_box_node(vmath.vector3(0,0,0), vmath.vector3(1,1,1))"
                    "    self.n2 = gui.new_text_node(vmath.vector3(0,0,0), \"My Node\")"
                    "end\n"
                    "function update(self)\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, ScriptInput)
{
    const char* s = "function update(self)\n"
                    "   assert(g_value == 123)\n"
                    "end\n"
                    "function on_input(self, action_id, action)\n"
                    "   if(action_id == hash(\"SPACE\")) then\n"
                    "       g_value = 123\n"
                    "   end\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::InputAction input_action;
    memset(&input_action, 0, sizeof(input_action));
    input_action.m_ActionId = dmHashString64("SPACE");
    dmGui::DispatchInput(m_Scene, &input_action, 1);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

struct TestMessage
{
    dmhash_t m_ComponentId;
    dmhash_t m_MessageId;
};

static void Dispatch1(dmMessage::Message* message, void* user_ptr)
{
    dmGui::MessageData* md = (dmGui::MessageData*) &message->m_Data[0];
    TestMessage* test_message = (TestMessage*) user_ptr;
    test_message->m_ComponentId = md->m_ComponentId;
    test_message->m_MessageId = md->m_MessageId;
}

TEST_F(dmGuiTest, PostMessage1)
{
    const char* s = "function init(self)\n"
                    "   gui.post_to(\"component\", \"my_named_message\")\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    TestMessage test_message;
    dmMessage::Dispatch(m_Socket, &Dispatch1, &test_message);

    ASSERT_EQ(dmHashString64("component"), test_message.m_ComponentId);
    ASSERT_EQ(dmHashString64("my_named_message"), test_message.m_MessageId);
}

TEST_F(dmGuiTest, MissingSetSceneInDispatchInputBug)
{
    const char* s = "function update(self)\n"
                    "end\n"
                    "function on_input(self, action_id, action)\n"
                    "   gui.post_to(\"component\", \"my_named_message\")\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::InputAction input_action;
    memset(&input_action, 0, sizeof(input_action));
    input_action.m_ActionId = dmHashString64("SPACE");
    r = dmGui::DispatchInput(m_Scene, &input_action, 1);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

static void Dispatch2(dmMessage::Message* message, void* user_ptr)
{
    dmGui::MessageData* md = (dmGui::MessageData*) &message->m_Data[0];
    assert(md->m_ComponentId == dmHashString64("component"));
    assert(md->m_DDFDescriptor == dmTestGuiDDF::AMessage::m_DDFDescriptor);

    dmTestGuiDDF::AMessage* amessage = (dmTestGuiDDF::AMessage*) md->m_DDFData;
    dmTestGuiDDF::AMessage* amessage_out = (dmTestGuiDDF::AMessage*) user_ptr;

    *amessage_out = *amessage;
}

TEST_F(dmGuiTest, PostMessage2)
{
    const char* s = "function init(self)\n"
                    "   gui.post_to(\"component\", \"a_message\", { a = 123, b = 456 })\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmTestGuiDDF::AMessage amessage;
    dmMessage::Dispatch(m_Socket, &Dispatch2, &amessage);

    ASSERT_EQ(123, amessage.m_A);
    ASSERT_EQ(456, amessage.m_B);
}

static void Dispatch3(dmMessage::Message* message, void* user_ptr)
{
    dmGui::MessageData* md = (dmGui::MessageData*) &message->m_Data[0];
    dmGui::Result r = dmGui::DispatchMessage((dmGui::HScene)user_ptr, md->m_MessageId, md->m_DDFData, md->m_DDFDescriptor);
    assert(r == dmGui::RESULT_OK);
}

TEST_F(dmGuiTest, PostMessage3)
{
    const char* s1 = "function init(self)\n"
                     "    gui.post_to(\"component\", \"test_message\", { a = 123 })\n"
                     "end\n";

    const char* s2 = "function update(self, dt)\n"
                     "    assert(self.a == 123)\n"
                     "end\n"
                     "\n"
                     "function on_message(self, message_id, message)\n"
                     "    if message_id == hash(\"test_message\") then\n"
                     "        self.a = message.a\n"
                     "    end\n"
                     "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s1, strlen(s1), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::NewSceneParams params;
    dmGui::HScene scene2 = dmGui::NewScene(m_Context, &params);
    ASSERT_NE((void*)scene2, (void*)0x0);
    dmGui::HScript script2 = dmGui::NewScript(m_Context);
    r = dmGui::SetSceneScript(scene2, script2);
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::SetScript(script2, s2, strlen(s2), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    uint32_t message_count = dmMessage::Dispatch(m_Socket, &Dispatch3, scene2);
    ASSERT_EQ(1u, message_count);

    r = dmGui::UpdateScene(scene2, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::DeleteScript(script2);
    dmGui::DeleteScene(scene2);
}

TEST_F(dmGuiTest, PostMessageMissingField)
{
    const char* s = "function init(self)\n"
                    "   gui.post_to(\"a_message\", { a = 123 })\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_SCRIPT_ERROR, r);
}

TEST_F(dmGuiTest, PostMessageToGuiDDF)
{
    const char* s = "local a = 0\n"
                    "function update(self)\n"
                    "   assert(a == 123)\n"
                    "end\n"
                    "function on_message(self, message_id, message)\n"
                    "   assert(message_id == hash(\"amessage\"))\n"
                    "   a = message.a\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmTestGuiDDF::AMessage amessage;
    amessage.m_A = 123;
    r = dmGui::DispatchMessage(m_Scene, dmHashString64("amessage"), &amessage, dmTestGuiDDF::AMessage::m_DDFDescriptor);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, PostMessageToGuiLuaTable)
{
    const char* s = "local a = 0\n"
                    "function update(self)\n"
                    "   assert(a == 456)\n"
                    "end\n"
                    "function on_message(self, message_id, message)\n"
                    "   assert(message_id == hash(\"amessage\"))\n"
                    "   a = message.a\n"
                    "end\n";

    char buffer[256];
    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    lua_State* L = lua_open();
    lua_newtable(L);
    lua_pushstring(L, "a");
    lua_pushinteger(L, 456);
    lua_settable(L, -3);
    uint32_t nused = dmScript::CheckTable(L, buffer, sizeof(buffer), -1);
    ASSERT_GT(nused, 0U);
    ASSERT_LE(nused, sizeof(buffer));

    r = dmGui::DispatchMessage(m_Scene, dmHashString64("amessage"), buffer, 0);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    lua_close(L);
}

TEST_F(dmGuiTest, SaveNode)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function init(self) self.n = gui.get_node(\"n\")\n end function update(self) print(self.n)\n end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, UseDeletedNode)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function init(self) self.n = gui.get_node(\"n\")\n end function update(self) print(self.n)\n end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::DeleteNode(m_Scene, node);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_SCRIPT_ERROR, r);
}

TEST_F(dmGuiTest, NodeProperties)
{
    dmGui::HNode node = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::SetNodeName(m_Scene, node, "n");
    const char* s = "function init(self)\n"
                    "self.n = gui.get_node(\"n\")\n"
                    "gui.set_position(self.n, vmath.vector4(1,2,3,0))\n"
                    "gui.set_text(self.n, \"test\")\n"
                    "gui.set_text(self.n, \"flipper\")\n"
                    "end\n"
                    "function update(self) "
                    "local pos = gui.get_position(self.n)\n"
                    "assert(pos.x == 1)\n"
                    "assert(pos.y == 2)\n"
                    "assert(pos.z == 3)\n"
                    "assert(gui.get_text(self.n) == \"flipper\")\n"
                    "end";
    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::DeleteNode(m_Scene, node);
}

TEST_F(dmGuiTest, ReplaceAnimation)
{
    /*
     * NOTE: We create a node2 which animation duration is set to 0.5f
     * Internally the animation will removed an "erased-swapped". Used to test that the last animation
     * for node1 really invalidates the first animation of node1
     */
    dmGui::HNode node1 = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);
    dmGui::HNode node2 = dmGui::NewNode(m_Scene, Point3(0,0,0), Vector3(10,10,0), dmGui::NODE_TYPE_BOX);

    dmGui::AnimateNode(m_Scene, node2, dmGui::PROPERTY_POSITION, Vector4(123,0,0,0), dmGui::EASING_NONE, 0.5f, 0, 0, 0, 0);
    dmGui::AnimateNode(m_Scene, node1, dmGui::PROPERTY_POSITION, Vector4(1,0,0,0), dmGui::EASING_NONE, 1.0f, 0, 0, 0, 0);
    dmGui::AnimateNode(m_Scene, node1, dmGui::PROPERTY_POSITION, Vector4(10,0,0,0), dmGui::EASING_NONE, 1.0f, 0, 0, 0, 0);

    for (int i = 0; i < 60; ++i)
    {
        dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    }

    ASSERT_NEAR(dmGui::GetNodePosition(m_Scene, node1).getX(), 10.0f, 0.001f);

    dmGui::DeleteNode(m_Scene, node1);
    dmGui::DeleteNode(m_Scene, node2);
}

TEST_F(dmGuiTest, SyntaxError)
{
    const char* s = "function_ foo(self)";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_SYNTAX_ERROR, r);
}

TEST_F(dmGuiTest, MissingUpdate)
{
    const char* s = "function init(self) end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, MissingInit)
{
    const char* s = "function update(self) end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, NoScript)
{
    dmGui::Result r;
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, Self)
{
    const char* s = "function init(self) self.x = 1122 end\n function update(self) assert(self.x==1122) end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, Reload)
{
    const char* s1 = "function init(self) self.x = 1122 end\n function update(self) assert(self.x==1122)\n self.x = self.x + 1 end";
    const char* s2 = "function update(self) assert(self.x==1123) end";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s1, strlen(s1), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    // assert should fail due to + 1
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_SCRIPT_ERROR, r);

    // Reload
    r = dmGui::SetScript(m_Script, s2, strlen(s2), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, ScriptNamespace)
{
    // Test that "local" per file works, default lua behavior
    // The test demonstrates how to create file local variables by using the local keyword at top scope
    const char* s1 = "local x = 123\n local function f() return x end\n function update(self) assert(f()==123)\n end\n";
    const char* s2 = "local x = 456\n local function f() return x end\n function update(self) assert(f()==456)\n return x\n end\n";

    dmGui::NewSceneParams params;
    dmGui::HScene scene2 = dmGui::NewScene(m_Context, &params);

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s1, strlen(s1), "file1");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::SetScript(m_Script, s2, strlen(s2), "file2");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
    r = dmGui::UpdateScene(scene2, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    dmGui::DeleteScene(scene2);
}

TEST_F(dmGuiTest, DeltaTime)
{
    const char* s = "function update(self, dt)\n"
                    "assert (dt == 1122)\n"
                    "end\n";

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, s, strlen(s), "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1122);
    ASSERT_EQ(dmGui::RESULT_OK, r);
}

TEST_F(dmGuiTest, Bug352)
{
    dmGui::AddFont(m_Scene, "big_score", 0);
    dmGui::AddFont(m_Scene, "score", 0);
    dmGui::AddTexture(m_Scene, "left_hud", 0);
    dmGui::AddTexture(m_Scene, "right_hud", 0);

    dmGui::Result r;
    r = dmGui::SetScript(m_Script, BUG352_LUA, BUG352_LUA_SIZE, "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);

    r = dmGui::SetScript(m_Script, BUG352_LUA, BUG352_LUA_SIZE, "file");
    ASSERT_EQ(dmGui::RESULT_OK, r);

    char buffer[256];
    lua_State* L = lua_open();
    lua_newtable(L);
    lua_pushstring(L, "score");
    lua_pushinteger(L, 123);
    lua_settable(L, -3);

    uint32_t nused = dmScript::CheckTable(L, buffer, sizeof(buffer), -1);
    ASSERT_GT(nused, 0U);
    ASSERT_LE(nused, sizeof(buffer));

    for (int i = 0; i < 100; ++i)
    {
        dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
        dmGui::DispatchMessage(m_Scene, dmHashString64("inc_score"), buffer, 0);
    }

    r = dmGui::UpdateScene(m_Scene, 1.0f / 60.0f);
    ASSERT_EQ(dmGui::RESULT_OK, r);
    lua_close(L);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
