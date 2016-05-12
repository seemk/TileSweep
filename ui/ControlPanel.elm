port module ControlPanel exposing (..)

import Html exposing (..)
import Html.App as App
import Html.Events exposing (..)
import Html.Attributes exposing (..)
import Debug exposing (..)
import String
import Http
import Task exposing (..)
import Json.Decode as JD exposing ((:=))
import Json.Encode as JE
import Time exposing (Time, second)

main =
  App.program
    { init = init, 
      view = view,
      update = update,
      subscriptions = subscriptions
    }

port bounds : ((List Coordinate) -> msg) -> Sub msg

type alias Coordinate = (Float, Float)

type Msg = Submit
  | Coordinates (List Coordinate)
  | SizeChange Int
  | MinZoomUpdate String
  | MaxZoomUpdate String
  | ResponseComplete StatusResponse
  | ResponseFail Http.Error
  | ServerStatusComplete ServerStatus
  | Tick Time

type alias ServerStatus =
  { requestQueueSize : Int
  , writeQueueSize : Int
  }

type alias StatusResponse =
  { status: Int }

type alias Model =
  { bounds : (List Coordinate)
  , size : Int
  , minZoom : Int
  , maxZoom : Int
  , serverStatus : ServerStatus
  }

init : (Model, Cmd Msg)
init =
  (Model [] 256 0 15 (ServerStatus 0 0), Cmd.none)

limitZoom z =
  if z > 19 then
    19
  else if z < 0 then
    0
  else
    z

getZoom string =
  limitZoom (Result.withDefault 0 (String.toInt string))

update : Msg -> Model -> (Model, Cmd Msg)
update msg model =
  case msg of
    Coordinates coords ->
      ({ model | bounds = coords }, Cmd.none)
    Submit ->
      (model, submitRenderJob model)
    SizeChange size ->
      ({ model | size = size }, Cmd.none)
    MinZoomUpdate z ->
      let zoom = Basics.clamp 0 model.maxZoom (getZoom z) in
      ({ model | minZoom = zoom }, Cmd.none)
    MaxZoomUpdate z ->
      let zoom = Basics.clamp model.minZoom 19 (getZoom z) in
      ({ model | maxZoom = zoom }, Cmd.none)
    ResponseComplete status ->
      (model, Cmd.none)
    ResponseFail _ ->
      (model, Cmd.none)
    Tick time ->
      (model, fetchServerInfo model)
    ServerStatusComplete response ->
      log (toString response)
      (model, Cmd.none)

status : JD.Decoder StatusResponse
status =
  JD.object1 StatusResponse ("status" := JD.int)

query m =
  JE.object
    [ ("width", JE.int m.size)
    , ("height", JE.int m.size)
    , ("zoom", JE.list (List.map JE.int [m.minZoom..m.maxZoom]))
    , ("bounds", JE.list (List.map (\(x,y) -> JE.list [JE.float x, JE.float y]) m.bounds))
    ]

serverStatus : JD.Decoder ServerStatus
serverStatus =
  JD.object2 ServerStatus ("requestQueueSize" := JD.int) ("writeQueueSize" := JD.int)

submitRenderJob : Model -> Cmd Msg
submitRenderJob m =
  Task.perform ResponseFail ResponseComplete (Http.post status "/prerender" (Http.string (JE.encode 0 (query m))))

fetchServerInfo : Model -> Cmd Msg
fetchServerInfo m =
  Task.perform ResponseFail ServerStatusComplete (Http.get serverStatus "/status")

subscriptions : Model -> Sub Msg
subscriptions model =
  Sub.batch [bounds Coordinates, Time.every 2000 Tick]

view : Model -> Html Msg
view model =
  div []
    [ div [ style [("display", "flex")] ] [ text "Tile size: ", radio 256 model, radio 512 model ]
    , div []
      [ div [] [ text "Zoom levels: " ] 
      , input [ type' "number", value (toString model.minZoom), onInput MinZoomUpdate ] []
      , input [ type' "number", value (toString model.maxZoom), onInput MaxZoomUpdate ] []
      ]
    , button [ onClick Submit ] [ text "Submit" ]
    , div [] [ text "Server status"
             , div [] [ text ("Request queue size " ++ (toString model.serverStatus.requestQueueSize)) ]
             ]
    ]

radio : Int -> Model -> Html Msg
radio value model =
  let isSelected =
    model.size == value
  in
    div [ style [("padding", "0 5px 0 5px")] ]
      [ input [ type' "radio", checked isSelected, onCheck (\_ -> SizeChange value) ] []
      , text (toString value)
      ]
