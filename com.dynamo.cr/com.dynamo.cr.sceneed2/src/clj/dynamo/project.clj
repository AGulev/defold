(ns dynamo.project
  (:require [clojure.java.io :as io]
            [schema.core :as s]
            [dynamo.system :as ds]
            [dynamo.file :as file]
            [dynamo.node :as n :refer [defnode Scope]]
            [dynamo.ui :as ui]
            [internal.clojure :as clojure]
            [internal.query :as iq]
            [internal.system :as is]
            [eclipse.resources :as resources]
            [service.log :as log])
  (:import [org.eclipse.core.resources IFile IProject IResource]
           [org.eclipse.ui PlatformUI]
           [org.eclipse.ui.internal.registry FileEditorMapping EditorRegistry]))

(set! *warn-on-reflection* true)

(defn register-filetype
  [extension default?]
  (let [reg ^EditorRegistry (.getEditorRegistry (PlatformUI/getWorkbench))
        desc (.findEditor reg "com.dynamo.cr.sceneed2.scene-editor")
        mapping (doto
                  (FileEditorMapping. extension)
                  (.addEditor desc))
        all-mappings (.getFileEditorMappings reg)]
    (when default? (.setDefaultEditor mapping desc))
    (ui/swt-safe (.setFileEditorMappings reg (into-array (concat (seq all-mappings) [mapping]))))))

(defn- handle
  [key filetype handler]
  (ds/update-property (ds/current-scope) :handlers assoc-in [key filetype] handler))

(defn register-loader
  [filetype loader]
  (handle :loader filetype loader))

(defn register-editor
  [filetype editor-builder]
  (handle :editor filetype editor-builder)
  (register-filetype filetype true))

(def default-handlers {:loader {"clj" clojure/on-load-code}})

(def no-such-handler
  (memoize
    (fn [key ext]
      (fn [& _] (throw (ex-info (str "No " (name key) " has been registered that can handle " ext) {}))))))

(defn- handler
  [project-scope key ext]
  (or
    (get-in project-scope [:handlers key ext])
    (no-such-handler key ext)))

(defn loader-for [project-scope ext] (handler project-scope :loader ext))
(defn editor-for [project-scope ext] (handler project-scope :editor ext))

(defn- load-resource
  [project-scope path]
  (ds/transactional
    (ds/in project-scope
      ((loader-for project-scope (file/extension path)) path (io/reader path)))))

(defn node-by-filename
  [project-scope filename]
  (load-resource project-scope (file/project-path project-scope filename)))

(defn- send-project-scope-message
  [graph self txn]
  (doseq [n (:nodes-added txn)]
    (ds/send-after n {:type :project-scope :scope self})))

; ---------------------------------------------------------------------------
; Lifecycle, Called by Eclipse
; ---------------------------------------------------------------------------
(defnode Project
  (inherits Scope)

  (property triggers {:schema s/Any :default [#'n/inject-new-nodes #'send-project-scope-message]})
  (property tag {:schema s/Keyword :default :project})
  (property eclipse-project IProject)
  (property branch String))

(defn open-project
  [eclipse-project branch]
  (ds/transactional
    (let [project-node    (ds/add (make-project :eclipse-project eclipse-project :branch branch :handlers default-handlers))
          clojure-sources (filter clojure/clojure-source? (resources/resource-seq eclipse-project))]
      (ds/in project-node
        (doseq [source clojure-sources]
          (load-resource project-node (file/project-path project-node source))))
      project-node)))

; ---------------------------------------------------------------------------
; Documentation
; ---------------------------------------------------------------------------
(doseq [[v doc]
       {*ns*
        "Functions for performing transactional changes to a project and inspecting its current state."

        #'open-project
        "Called from com.dynamo.cr.editor.Activator when opening a project. You should not call this function."

        #'register-filetype
        "TODO"

        #'register-loader
        "Associate a filetype (extension) with a loader function. The given loader will be
used any time a file with that type is opened."

        #'register-editor
        "TODO"

        #'editor-for
        "TODO"

        #'load-resource
        "Load a resource, usually from file. This looks up a suitable loader based on the filename.
Loaders must be registered via register-loader before they can be used.

This will invoke the loader function with the filename and a reader to supply the file contents."

        #'node-by-filename
        "TODO"

        #'Project
        "TODO"
}]
  (alter-meta! v assoc :doc doc))
