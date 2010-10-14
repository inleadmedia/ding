<?php
// $Id$

/**
 * Reserve an item immediately.
 */
function ding_reservation_page_reserve_item($object, $reservable = NULL) {
  global $user;
  $is_ahah = ding_reservation_detect_ahah();

  // If client wants JSON (usually AJAX requests), let's give it to them.
  if ($is_ahah) {
    $reply = array();
    $result = ding_provider_invoke('reservation', 'create', $user, $item_id);
    switch ($result['status']) {
      case DING_RESERVATION_SUCCESS:
        $reply['status'] = 'success';
        $reply['title'] = t('Success');
        $reply['message'] = isset($result['branch']) ?
          t('You have reserved @item and will be notified when its ready for pickup at @branch.', array('@item' => $object->title, '@branch' => $result['branch'])) :
          t('You have reserved @item and will be notified when its ready for pickup.', array('@item' => $object->title));
        $reply['cart_link'] = url('user/' . $user->uid . '/status');
        break;
      case DING_PROVIDER_AUTH_REQUIRED:
        $reply['status'] = 'error';
        $reply['error_type'] = 'not-logged-in';
        $reply['title'] = t('Not logged in');
        $reply['message'] = t('You have to log in to make reservations.');
        break;
      case DING_RESERVATION_ERROR_NO_DEFAULT_BRANCH:
        $reply['status'] = 'error';
        $reply['error_type'] = 'no-preferred-branch';
        $reply['title'] = t('Preferred branch not set');
        $reply['message'] = t('To make quick reservations, you need to have a preferred branch set. You can set one up at your !profile_settings.',
                            array('!profile_settings' => l(t('profile settings page'), 'user/' . $user->uid . '/profile', array('query' => drupal_get_destination()))));
        break;
      case DING_RESERVATION_ERROR_RESERVATION_EXISTS:
        $reply['status'] = 'error';
        $reply['error_type'] = 'reservation-exists';
        $reply['title'] = t('Reservation exists');
        $reply['message'] = t('@item is already reserved.', array('@item' => $object->title));
        break;
      default:
        $reply['status'] = 'error';
        $reply['error_type'] = 'reservation-error';
        $reply['title'] = t('Reservation error');
        $reply['message'] = t('An error occurred while reserving item. Please try again.');
        break;
    }

    // We are returning JavaScript, so tell the browser.
    drupal_set_header('Content-Type: application/json; charset=utf-8');
    echo json_encode($reply);
    return;
  }

  // Ensure that we're authenticated with the backend, redirect if not.
  ding_library_user_required();
  ting_enrich_object($object);

  if ($reservation_exists && $reservation_exists != DING_PROVIDER_AUTH_REQUIRED) {
    drupal_set_message(t('@item is already reserved.', array('@item' => $object->title)), 'warning');
    return drupal_goto('user/' . $user->uid . '/status');
  }

  return drupal_get_form('ding_reservation_confirm_form', $user, $object);
}

/**
 * Ding reservation confirmation form.
 *
 * @param array $form_state
 *    Drupal form API state.
 * @param stdClass $account
 *    Drupal user account.
 * @param TingClientObject $object
 *    The object we're acting on.
 */
function ding_reservation_confirm_form(&$form_state, $account, $object) {
  // Send some form values along for the submit step.
  $form = array();

  $form['account'] = array(
    '#type' => 'value',
    '#value' => $account,
  );

  $form['object'] = array(
    '#type' => 'value',
    '#value' => $object,
  );

  $form['dest_url'] = array(
    '#type' => 'value',
    '#value' => $urls[$action],
  );

  return confirm_form($form, t('Reserve %book immediately?', array('%book' => $object->title)), 'user/' . $account->uid . '/status', '');
}

/**
 * Submit handler for Ding cart confirmation form.
 *
 * @todo:
 * Do we really need a confirm form? If we do, it should show
 * reservation options too.
 */
function ding_reservation_confirm_form_submit($form, &$form_state) {
  $reservation = ding_provider_invoke('reservation', 'create', $form_state['values']['account'], $form_state['values']['object']->localId, array('pickup_branch' => $form_state['values']['account']->preferred_branch));

  if ($reservation && $reservation != DING_PROVIDER_AUTH_REQUIRED) {
    drupal_set_message(t('@item is now on your reservation list.', array('@item' => $form_state['values']['object']->title)));
  }
  else {
    drupal_set_message(t('Reservation of @item failed.', array('@item' => $form_state['values']['object']->title)));
  }
  break;

  $form_state['redirect'] = $form_state['values']['dest_url'];
}

/**
 * Detect if a request wants a JSON-response, usually for AHAH.
 */
function ding_reservation_detect_ahah() {
  return (isset($_SERVER['HTTP_ACCEPT']) && (strpos($_SERVER['HTTP_ACCEPT'], 'application/json')) || (strpos($_SERVER['HTTP_ACCEPT'], 'text/javascript')));
}