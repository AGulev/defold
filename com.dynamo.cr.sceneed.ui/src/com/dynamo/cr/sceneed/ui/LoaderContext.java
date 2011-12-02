package com.dynamo.cr.sceneed.ui;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;

import com.dynamo.cr.editor.core.ILogger;
import com.dynamo.cr.editor.core.IResourceType;
import com.dynamo.cr.sceneed.core.INodeType;
import com.dynamo.cr.sceneed.core.INodeTypeRegistry;
import com.dynamo.cr.sceneed.core.ISceneView.INodeLoader;
import com.dynamo.cr.sceneed.core.Node;
import com.google.inject.Inject;

public class LoaderContext implements
com.dynamo.cr.sceneed.core.ISceneView.ILoaderContext {

    private final IContainer contentRoot;
    private final INodeTypeRegistry nodeTypeRegistry;
    private final ILogger logger;

    @Inject
    public LoaderContext(IContainer contentRoot, INodeTypeRegistry nodeTypeRegistry, ILogger logger) {
        this.contentRoot = contentRoot;
        this.nodeTypeRegistry = nodeTypeRegistry;
        this.logger = logger;
    }

    @Override
    public void logException(Throwable exception) {
        this.logger.logException(exception);
    }

    @Override
    public Node loadNode(String path) throws IOException, CoreException {
        if (path != null && !path.equals("")) {
            IFile file = this.contentRoot.getFile(new Path(path));
            if (file.exists()) {
                return loadNode(file.getFileExtension(), file.getContents());
            }
        }
        return null;
    }

    @Override
    public Node loadNode(String extension, InputStream contents)
            throws IOException, CoreException {
        INodeType nodeType = this.nodeTypeRegistry.getNodeTypeFromExtension(extension);
        if (nodeType != null) {
            INodeLoader<Node> loader = nodeType.getLoader();
            return loader.load(this, contents);
        }
        return null;
    }

    @Override
    public Node loadNodeFromTemplate(Class<? extends Node> nodeClass) throws IOException, CoreException {
        INodeType nodeType = this.nodeTypeRegistry.getNodeType(nodeClass);
        if (nodeType != null) {
            String extension = nodeType.getExtension();
            return loadNodeFromTemplate(extension);
        }
        return null;
    }

    @Override
    public Node loadNodeFromTemplate(String extension) throws IOException, CoreException {
        INodeType nodeType = this.nodeTypeRegistry.getNodeTypeFromExtension(extension);
        if (nodeType != null) {
            IResourceType resourceType = nodeType.getResourceType();
            ByteArrayInputStream stream = new ByteArrayInputStream(resourceType.getTemplateData());
            return loadNode(extension, stream);
        }
        return null;
    }

    @Override
    public INodeTypeRegistry getNodeTypeRegistry() {
        return this.nodeTypeRegistry;
    }

}
