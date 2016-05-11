port module ControlPanel exposing (..)

import Html exposing (..)
import Html.App as App
import Html.Events exposing (..)
import Html.Attributes exposing (..)
import Debug exposing (..)
import String

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

type alias Model =
  { bounds : (List Coordinate)
  , size : Int
  , zoomLevels : (List Int)
  , minZoom : Int
  , maxZoom : Int
  }

init : (Model, Cmd Msg)
init =
  (Model [] 256 [1..15] 0 15, Cmd.none)

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
      (model, Cmd.none)
    SizeChange size ->
      ({ model | size = size }, Cmd.none)
    MinZoomUpdate z ->
      let zoom = Basics.clamp 0 model.maxZoom (getZoom z) in
      ({ model | minZoom = zoom }, Cmd.none)
    MaxZoomUpdate z ->
      let zoom = Basics.clamp model.minZoom 19 (getZoom z) in
      ({ model | maxZoom = zoom }, Cmd.none)


subscriptions : Model -> Sub Msg
subscriptions model =
  bounds Coordinates

view : Model -> Html Msg
view model =
  div []
    [ div [ style [("display", "flex")] ] [ text "Tile size: ", radio 256 model, radio 512 model ]
    , input [ type' "number", value (toString model.minZoom), onInput MinZoomUpdate ] []
    , input [ type' "number", value (toString model.maxZoom), onInput MaxZoomUpdate ] []
    , button [ onClick Submit ] [ text "Submit" ]
    , h1 [] [ text (toString (List.length model.bounds)) ]
    , h1 [] [ text (toString model.size) ]
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
